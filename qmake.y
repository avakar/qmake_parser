%include {
#include "ast.hpp"
}

%context_lexer

WS :: discard
WS ~= {[ \t\v\f]+}
WS ~= {\\\n}
WS ~= {#[^\n]*}

NL ~= {\n}

block :: {std::shared_ptr<block_stmt>}
block ::= . { return std::make_shared<block_stmt>(); }
block ::= block NL.
block(r) ::= block(r) cond_stmt(cs) NL. { r->stmts.push_back(cs); }
block(r) ::= block(r) cond_list(c1) cond(c2) "{" NL block(nested) "}". {
	cond_stmt cs;
	cs.c = c1;
	cs.c.push_back(c2);
	cs.s = nested;
	r->stmts.push_back(cs);
}

cond_stmt :: {cond_stmt}
cond_stmt(r) ::= cond_list(c) uncond_stmt(s). {
	r.c = c;
	r.s = s;
}

uncond_stmt :: {std::shared_ptr<stmt>}
uncond_stmt ::= var_stmt.
uncond_stmt ::= fncall(c). {
	std::shared_ptr<fncall_stmt> r = std::make_shared<fncall_stmt>();
	r->call = c;
	return r;
}

fncall :: {fncall}
fncall(r) ::= IDENT(name) "(" param_list(args) ")". {
	r.fn = name;
	r.args = args;
}

param_list :: {std::vector<std::string>}
param_list(r) ::= . {}
param_list(r) ::= PARAM_TEXT(i). { r.push_back(i); }
param_list(r) ::= param_list(r) "," PARAM_TEXT(i). { r.push_back(i); }

cond_list :: {std::vector<std::vector<cond> >}
cond_list(r) ::= . {}
cond_list(r) ::= cond_list(r) cond(c) ":". { r.push_back(c); }

cond :: {std::vector<cond>}
cond(r) ::= cond_atom(a). { r.push_back(a); }
cond(r) ::= cond(r) "|" cond_atom(a). { r.push_back(a); }

cond_atom :: {cond}
cond_atom(r) ::= "!" cond_atom(r). {
	r.invert = !r.invert;
}
cond_atom(r) ::= IDENT(o). {
	r.invert = false;
	r.call.fn = o;
}
cond_atom(r) ::= fncall(c). {
	r.invert = false;
	r.call = c;
}

var_stmt :: {std::shared_ptr<stmt>}
var_stmt ::= IDENT(name) var_op(kind) ident_list(l). {
	std::shared_ptr<var_stmt> r = std::make_shared<var_stmt>();
	r->name = name;
	r->kind = kind;
	r->ident_list = l;
	return r;
}

var_op :: {var_stmt::kind_t}
var_op ::= "=". { return var_stmt::k_eq; }
var_op ::= "+=". { return var_stmt::k_add; }
var_op ::= "-=". { return var_stmt::k_sub; }
var_op ::= "*=". { return var_stmt::k_add_unique; }
var_op ::= "~=". { return var_stmt::k_regex; }

ident_list :: {std::vector<std::string>}
ident_list(l) ::= . {}
ident_list(l) ::= ident_list(l) list_item(i). { l.push_back(i); }

list_item :: {std::string}
list_item ::= TEXT.
list_item ::= STR.

STR :: {std::string}
STR ~= {"([^"]|\\")*"}(s) { return s.substr(1, s.size() - 2); }

IDENT :: {std::string}
IDENT ~= {([^"\s\(\)\{\},:!=\+\|]|"([^"]|\\")*")+}

PARAM_TEXT :: {std::string}
PARAM_TEXT ~= {([^"\s,\(\)]|"([^"]|\\")*")+}

TEXT :: {std::string}
TEXT ~= {([^"\s]|"([^"]|\\")*")+}
