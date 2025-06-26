#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Mangle.h"
#include "clang/Sema/ParsedAttr.h"
#include "clang/Sema/Sema.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/Sema/SemaConsumer.h"
#include "llvm/IR/Attributes.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include <ranges>
#include <print>
#include <iostream>
#include <fstream>
#include <ranges>
namespace {

bool trace = 0;

struct diags {
  clang::DiagnosticsEngine *de;
  unsigned unknown_argument;
  unsigned missing_out_file;
  unsigned arg_non_expr;
  unsigned body_not_string;
  unsigned dep_not_global_lvalue;
} diag; // it's going to be filled out in ParseArgs
template<class F>
struct defer{
  F f;
  ~defer() {
    f();
  }
};

struct jsbind_consume: clang::SemaConsumer {
  std::ofstream out_file;
  std::string data;
  clang::Sema *s;
  std::optional<clang::PrintingPolicy> human_fn_name;
  clang::QualType str_type;
  std::unique_ptr<clang::ItaniumMangleContext> mangle_ctx;

  jsbind_consume(std::ofstream path, diags diag): out_file(std::move(path)) {}

  void InitializeSema(clang::Sema &s) override {
    this->s = &s;
    str_type = s.Context.getPointerType(
      s.Context.getConstType(s.Context.CharTy));
    mangle_ctx = std::unique_ptr<clang::ItaniumMangleContext>{
      clang::ItaniumMangleContext::create(s.Context, s.getDiagnostics())
    };
    human_fn_name = clang::PrintingPolicy(s.Context.getLangOpts()); 
    //human_fn_name->FullyQualifiedName = 1;
    human_fn_name->SuppressScope = 0;
    //human_fn_name->PrintCanonicalTypes = 1;
  }
  auto has_appropriate_attr(clang::FunctionDecl *d) -> decltype(std::ranges::begin(d->getAttrs())) {
    return std::ranges::find_if(d->getAttrs(), [d, this](const clang::Attr *att) {
        if (auto an = dyn_cast<clang::AnnotateAttr>(att)) {
          if(trace)
            std::println(std::cerr, "name: {}", make_human_name(d));
          auto name = an->getAnnotation();
          if(name == "__jsbind" || name == "__jsbind_dep")
            return true;
          }
        return false;
      });
  }
  bool non_template_p(clang::FunctionDecl *d) {
    auto dep = d->getTemplatedKind();
    using K = clang::FunctionDecl::TemplatedKind;
    return dep == K::TK_NonTemplate
      || dep == K::TK_MemberSpecialization
      || dep == K::TK_FunctionTemplateSpecialization;
  }

  void write(uint32_t v) {
    out_file.write((const char *)&v, sizeof v);
  }
  void write(std::string_view v) {
    size_t sz = size(v);
    write(sz);
    out_file.write(v.data(), sz);
  }

  clang::NamedDecl *evaluate_decl(clang::Expr *e) {
    clang::Expr::EvalResult res;
    e->EvaluateAsLValue(res, s->Context);
    if (!res.isGlobalLValue()) {
      diag.de->Report(e->getExprLoc(), diag.dep_not_global_lvalue);
      return 0;
    }
    return const_cast<clang::ValueDecl *>(res.Val.getLValueBase().get<const clang::ValueDecl *>());
  }

  auto evaluate_str(clang::Expr *arg, bool dep) {
    if (dep) {
      auto res = s->BuildConvertedConstantExpression(arg, str_type, clang::Sema::CCEK_TemplateArg);
      if (res.isUsable())
        arg = res.get();
      else return std::optional<std::string>{};
    }
    return arg->tryEvaluateString(s->Context);
  }
  std::string mangle(const clang::NamedDecl *f) {
    std::string name;
    llvm::raw_string_ostream name_stream{name};
    mangle_ctx->mangleName(f, name_stream);
    return name;
  }
  std::string make_human_name(const clang::NamedDecl *f) {
    std::string name;
    llvm::raw_string_ostream name_stream{name};
    f->getNameForDiagnostic(name_stream, *human_fn_name, true);
    return name;
  }

  void handle(clang::FunctionDecl *d) {
    auto attr_it = has_appropriate_attr(d);
    if(attr_it == d->getAttrs().end()) return;
    if(!non_template_p(d)) return;
    auto attr = dyn_cast<clang::AnnotateAttr>(*attr_it);
    defer _{[&]{
        d->getAttrs().erase(attr_it);
    }};

    bool fail = false;
    clang::Expr *arg = attr->args_begin()[0];
    auto str = evaluate_str(arg, attr->getAnnotation() == "__jsbind_dep");
    if (!str) {
      diag.de->Report(arg->getExprLoc(), diag.body_not_string);
      fail = true;
    }
    auto name = mangle(d);
    auto deps_rg = 
      attr->args()
      | std::views::drop(1)
      | std::views::transform([this, &fail, d](clang::Expr *arg) {
        auto decl = evaluate_decl(arg);
        decl->markUsed(d->getASTContext());
        if (decl) return mangle(decl);
        fail = true;
        return std::string{};
      });
    std::vector<std::string> deps(std::begin(deps_rg), std::end(deps_rg));

    if(!fail) {
      write(name);
      write(make_human_name(d));
      write(*str);
      write(size(deps));
      for(auto &&dep: deps) {
        write(dep);
      }
      if(trace)
        std::println(std::cerr, "\"{}\", \"{}\"", *str, size(deps));
    }
  }

  void HandleCXXImplicitFunctionInstantiation(
    clang::FunctionDecl *d
  ) override {
    handle(d);
  }

  bool HandleTopLevelDecl(clang::DeclGroupRef dg) override {
    for (auto decl: dg) {
      //if (auto *dn = dyn_cast<clang::NamedDecl>(decl))
        //std::println(std::cerr, "completeme: {}", make_human_name(dn));
      if (auto *dn = dyn_cast<clang::NamespaceDecl>(decl)) {
        //std::println(std::cerr, "here");
        if (!dn->hasBody()) continue;
        //std::println(std::cerr, "here!!");
        
        auto body = dyn_cast<clang::CompoundStmt>(dn->getBody());
        for(auto stmt: body->body())
          if(auto dn = dyn_cast<clang::DeclStmt>(stmt))
            HandleTopLevelDecl(dn->getDeclGroup());
      }

      auto *d = dyn_cast<clang::FunctionDecl>(decl);
      if(!d) continue;
      handle(d);
    }
    return true;
  }
  void HandleInterestingDecl(clang::DeclGroupRef dg) override {
    for (auto decl: dg) {
      auto *d = dyn_cast<clang::FunctionDecl>(decl);
      if(!d) continue;
      handle(d);
    }
  }
  void CompleteExternalDeclaration(clang::DeclaratorDecl *dg) override {
    auto *d = dyn_cast<clang::FunctionDecl>(dg);
    if(!d) return;
    //std::println(std::cerr, "completeme: {}", make_human_name(d));
    handle(d);
  }

  struct action: clang::PluginASTAction {
    std::string out_path;
  protected:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &CI, llvm::StringRef
    ) override {
      return std::make_unique<jsbind_consume>(std::ofstream{out_path, std::ios::binary | std::ios::trunc}, diag);
    }
    PluginASTAction::ActionType getActionType() override {
      return AddBeforeMainAction;
    }
    bool ParseArgs(
      const clang::CompilerInstance &inst,
      const std::vector<std::string> &args
    ) override {
      using DE = clang::DiagnosticsEngine;

      diag.de = &inst.getDiagnostics();
      diag.unknown_argument = diag.de->getCustomDiagID(DE::Error, "unknown argument: %0");
      diag.missing_out_file = diag.de->getCustomDiagID(DE::Error, "metadata putput file is missing");
      diag.arg_non_expr = diag.de->getCustomDiagID(DE::Error, "argument must be an expression but parsed as an identifier");
      diag.body_not_string = diag.de->getCustomDiagID(DE::Error, "JavaScript body is not a null-terminated string");
      diag.dep_not_global_lvalue = diag.de->getCustomDiagID(DE::Error, "JavaScript body cannot depend on non-globals");
      
      constexpr static char file_path_arg[] = "out-file-path=";
      bool fail = false;
      for (std::string_view arg: args) {
        if (arg == "trace")
          trace = 1;
        if (arg.starts_with(file_path_arg)) {
          arg.remove_prefix(sizeof file_path_arg - 1);
          if(trace)
            std::println("file:{}", arg);
          out_path = arg;
        }
        else {
          diag.de->Report(diag.unknown_argument) << arg;
          fail = true;
        }
      }
      if (out_path == "") {
        diag.de->Report(diag.missing_out_file);
        fail = true;
      }
      return !fail;
    }
  };
};

struct jsbind_attr: clang::ParsedAttrInfo {
  constexpr static Spelling s[] = {
    {clang::ParsedAttr::AS_CXX11, "jsbind::jsbind"},
    {clang::ParsedAttr::AS_CXX11, "jsbind"},
    {clang::ParsedAttr::AS_C23, "jsbind"},
    {clang::ParsedAttr::AS_GNU, "jsbind"},
  };
  jsbind_attr():clang::ParsedAttrInfo(clang::AttributeCommonInfo::NoSemaHandlerAttribute, 1, 15, 0, 0, 1, 0, 0, 0, 0, 0, s, {}) {
    NumArgs = 1;
    OptArgs = 15;
    AcceptsExprPack = 1;
    Spellings = s;
  }
  bool isParamExpr(size_t N) const override { return true; }
  bool diagAppertainsToDecl(
    clang::Sema &s, const clang::ParsedAttr &attr, const clang::Decl *d
  ) const override {
    auto *cast = dyn_cast<clang::FunctionDecl>(d);
    if(!cast){
      s.Diag(attr.getLoc(), clang::diag::warn_attribute_wrong_decl_type)
        << attr << attr.isRegularKeywordAttribute() << clang::ExpectedFunction;
      return false;
    }
    return true;
  }
  AttrHandling handleDeclAttribute(
    clang::Sema &s, clang::Decl *d, const clang::ParsedAttr &attr
  ) const override {
    auto *cast = dyn_cast<clang::FunctionDecl>(d);
    if(!cast){ return AttributeNotApplied; }
    
    std::vector<clang::Expr *> args(attr.getNumArgs());
    bool fail = false;
    for (auto &&[idx, out]: std::views::enumerate(args)) {
      auto arg = attr.getArg(idx);
      if (auto expr = dyn_cast<clang::Expr *>(arg))
        out = expr;
      else {
        auto ident = clang::cast<clang::IdentifierLoc *>(arg);
        diag.de->Report(ident->Loc, diag.arg_non_expr);
        fail = true;
      }
    }
    if (fail) return NotHandled;
    auto str_type = s.Context.getPointerType(
      s.Context.getConstType(s.Context.CharTy));
    auto dep = args[0]->getDependence();
    if (dep & decltype(dep)::Error)
      return NotHandled;
    else if (dep) {
      d->addAttr(clang::AnnotateAttr::Create(s.Context, "__jsbind_dep", args.data(), args.size(), attr));
    }
    else  {
      auto res = s.BuildConvertedConstantExpression(args[0], str_type, clang::Sema::CCEK_TemplateArg);
      if (res.isUsable())
        args[0] = res.get();
      else 
        return NotHandled;
      d->addAttr(clang::AnnotateAttr::Create(s.Context, "__jsbind", args.data(), args.size(), attr));
    }
    return AttributeApplied;
  }
  bool diagAppertainsToStmt(
    clang::Sema &s, const clang::ParsedAttr &attr, const clang::Stmt *d
  ) const override {
    return false;
  }
};
}
static clang::ParsedAttrInfoRegistry::Add<jsbind_attr> X{"jsbind_attr", "add custom sections for jsbind functions"};
static clang::FrontendPluginRegistry::Add<jsbind_consume::action> Y{"jsbind", "add custom sections for jsbind functions"};
