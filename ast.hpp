#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <set>
#include <memory>

struct fncall
{
	std::string fn;
	std::vector<std::string> args;
};

struct cond
{
	bool invert;
	fncall call;
};

struct stmt
{
	virtual ~stmt() {}
};

struct cond_stmt
{
	std::vector<std::vector<cond> > c; 
	std::shared_ptr<stmt> s;
};

struct var_stmt
	: stmt
{
	enum kind_t { k_eq, k_add, k_sub, k_add_unique, k_regex };

	std::string name;
	kind_t kind;
	std::vector<std::string> ident_list;
};

struct fncall_stmt
	: stmt
{
	fncall call;
};

struct block_stmt
	: stmt
{
	std::vector<cond_stmt> stmts;
};

#endif // AST_HPP
