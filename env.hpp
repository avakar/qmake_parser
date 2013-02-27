#ifndef ENV_HPP
#define ENV_HPP

#include "ast.hpp"
#include <map>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem; // XXX

class env_t;

bool process_qmake_file(std::string const & fname, env_t & env);
env_t process_root_qmake_file(std::string const & fname);

struct process_context
{
	process_context(std::string const & filename)
		: filename(filename)
	{
		dir = fs::path(filename).remove_filename().string();
	}

	std::string filename;
	std::string dir;
};

class env_t
{
public:
	typedef std::map<std::string, std::vector<std::string> >::const_iterator const_iterator;
	const_iterator begin() const { return vars.begin(); }
	const_iterator end() const { return vars.end(); }

	env_t()
	{
	}

	void process_stmt(process_context & ctx, stmt * s)
	{
		if (block_stmt * b = dynamic_cast<block_stmt *>(s))
			process_block_stmt(ctx, b);
		else if (var_stmt * b = dynamic_cast<var_stmt *>(s))
			process_var_stmt(ctx, *b);
		else if (fncall_stmt * b = dynamic_cast<fncall_stmt *>(s))
			process_fncall_stmt(ctx, *b);
	}

	void process_block_stmt(process_context & ctx, block_stmt * b)
	{
		bool last_enabled = false;
		for (size_t i = 0; i < b->stmts.size(); ++i)
		{
			last_enabled = check_condition(b->stmts[i].c, last_enabled);
			if (last_enabled)
				process_stmt(ctx, b->stmts[i].s.get());
		}
	}

	void add_var(std::string const & name, std::string const & val)
	{
		vars[name].push_back(val);
	}

	void set_var(std::string const & name, std::string const & val)
	{
		auto & v = vars[name];
		v.clear();
		v.push_back(val);
	}

	void process_var_stmt(process_context & ctx, var_stmt const & s)
	{
		std::vector<std::string> processed_values;
		for (size_t i = 0; i < s.ident_list.size(); ++i)
			processed_values.push_back(translate_value(s.ident_list[i]));

		switch (s.kind)
		{
		case var_stmt::k_eq:
			vars[s.name] = processed_values;
			break;
		case var_stmt::k_add:
			{
				std::vector<std::string> & val = vars[s.name];
				val.insert(val.end(), processed_values.begin(), processed_values.end());
			}
			break;
		case var_stmt::k_add_unique:
			{
				std::vector<std::string> & val = vars[s.name];
				for (size_t i = 0; i < processed_values.size(); ++i)
				{
					if (std::find(val.begin(), val.end(), processed_values[i]) != val.end())
						val.push_back(processed_values[i]);
				}
			}
			break;
		case var_stmt::k_sub:
			{
				std::vector<std::string> & val = vars[s.name];
				for (size_t i = 0; i < processed_values.size(); ++i)
				{
					auto it = std::find(val.begin(), val.end(), processed_values[i]);
					if (it != val.end())
						val.erase(it);
				}
			}
			break;
		}
	}

	void process_fncall_stmt(process_context & ctx, fncall_stmt const & b)
	{
		if (b.call.fn == "include" && b.call.args.size() == 1)
		{
			process_qmake_file(fs::absolute(this->translate_value(b.call.args[0]), ctx.dir).string(), *this);
		}
		else
		{
			throw std::runtime_error("Unkonwn function call: " + b.call.fn);
		}
	}

	std::vector<std::string> const * get(std::string const & name) const
	{
		auto it = vars.find(name);
		return it == vars.end()? nullptr: &it->second;
	}

	std::vector<std::string> get_many(std::string const & name) const
	{
		auto it = vars.find(name);
		if (it == vars.end())
			return std::vector<std::string>();
		return it->second;
	}

	std::string get_one(std::string const & name) const
	{
		auto const * v = this->get(name);
		if (!v || v->size() != 1)
			throw std::runtime_error("XXX");
		return (*v)[0];
	}

	struct vector_iter
	{
		std::string const * first;
		std::string const * last;

		vector_iter()
			: first(nullptr), last(nullptr)
		{
		}

		vector_iter(std::string const * first, std::string const * last)
			: first(first), last(last)
		{
		}

		std::string const * begin() const { return first; }
		std::string const * end() const { return last; }

		operator std::vector<std::string>() const
		{
			return std::vector<std::string>(first, last);
		}
	};

	vector_iter get_var_many(std::string const & name) const
	{
		std::string res;
		std::vector<std::string> const * v = this->get(name);
		if (!v)
			return vector_iter();
		return vector_iter(v->data(), v->data() + v->size());
	}

	std::string get_var(std::string const & name) const
	{
		std::string res;
		std::vector<std::string> const * v = this->get(name);
		if (!v)
			return res;
		for (size_t i = 0; i < v->size(); ++i)
		{
			if (i != 0)
				res.append(1, ' ');
			res.append((*v)[i]);
		}
		return res;
	}

	std::string get_env_var(std::string const & name) const
	{
		return getenv(name.c_str());
	}

	std::string get_prop(std::string const & name) const
	{
		if (name == "QT_INSTALL_HEADER")
			return "c:\\QtSDK\\Desktop\\Qt\\4.8.1\\msvc2010\\include";
		if (name == "QT_INSTALL_LIB")
			return "c:\\QtSDK\\Desktop\\Qt\\4.8.1\\msvc2010\\lib";
		return "";
	}

	std::string translate_value(std::string const & s) const
	{
		std::string res;
		res.reserve(s.size());

		char const * first = s.data();
		char const * last_store = first;
		char const * cur = first;
		char const * last = first + s.size();

		enum { st_idle, st_dollar, st_dollardollar, st_brace, st_bracket, st_paren } state = st_idle;
		while (cur != last)
		{
			switch (state)
			{
			case st_idle:
				if (*cur == '$')
				{
					res.append(last_store, cur);
					last_store = cur;
					state = st_dollar;
				}
				++cur;
				break;
			case st_dollar:
				if (*cur == '$')
				{
					state = st_dollardollar;
					first = cur + 1;
				}
				else
				{
					state = st_idle;
				}
				++cur;
				break;
			case st_dollardollar:
				if (*cur == '{' || *cur == '[' || *cur == '(')
				{
					if (first == cur)
					{
						state = *cur == '{'? st_brace: *cur == '['? st_bracket: st_paren;
						++cur;
					}
					else
						state = st_idle;
				}
				else if (isalnum((unsigned char)*cur) || *cur == '_')
				{
					++cur;
				}
				else
				{
					if (first != cur)
					{
						res.append(this->get_var(std::string(first, cur)));
						last_store = cur;
					}
					state = st_idle;
				}
				break;
			case st_brace:
			case st_bracket:
			case st_paren:
				if ((state == st_brace && *cur == '}') || (state == st_bracket && *cur == ']') || (state == st_paren && *cur == ')'))
				{
					std::string tmp(first + 1, cur);
					switch (state)
					{
					case st_brace: tmp = this->get_var(tmp); break;
					case st_bracket: tmp = this->get_prop(tmp); break;
					case st_paren: tmp = this->get_env_var(tmp); break;
					}

					res.append(tmp);
					last_store = cur + 1;
					state = st_idle;
				}
				++cur;
				break;
			}
		}

		if (state == st_dollardollar && first != cur)
			res.append(this->get_var(std::string(first, last)));
		else
			res.append(std::string(last_store, last));

		return res;
	}

private:
	static std::set<std::string> split_options(std::string const & u)
	{
		std::set<std::string> res;
		size_t start = 0;
		size_t pos = u.find('|');
		while (pos != std::string::npos)
		{
			res.insert(u.substr(start, pos - start));

			start = pos + 1;
			pos = u.find('|', start);
		}
		res.insert(u.substr(start));
		return res;
	}

	bool check_condition(std::vector<std::vector<cond> > const & cond_list, bool last_enabled)
	{
		bool enabled = true;
		for (size_t i = 0; enabled && i < cond_list.size(); ++i)
		{
			std::vector<cond> const & dc = cond_list[i];

			enabled = false;
			for (size_t j = 0; !enabled && j < dc.size(); ++j)
			{
				cond const & c = dc[j];

				if (c.call.fn == "else" && c.call.args.size() == 0)
				{
					enabled = !last_enabled;
				}
				else if (c.call.args.empty())
				{
					auto cfg = vars.find("CONFIG");
					enabled = cfg != vars.end()
						&& std::find(cfg->second.begin(), cfg->second.end(), c.call.fn) != cfg->second.end();
				}
				else if (c.call.fn == "CONFIG" && c.call.args.size() == 1)
				{
					auto cfg = vars.find("CONFIG");
					enabled = cfg != vars.end()
						&& std::find(cfg->second.begin(), cfg->second.end(), c.call.args[0]) != cfg->second.end();
				}
				else if (c.call.fn == "CONFIG" && c.call.args.size() == 2)
				{
					std::string const & option = c.call.args[0];
					enabled = false;
					auto cond_var_it = vars.find("CONFIG");
					if (cond_var_it != vars.end())
					{
						std::vector<std::string> const & opts = cond_var_it->second;
						std::set<std::string> const & option_universe = split_options(c.call.args[1]);
						for (size_t i = 0; i < opts.size(); ++i)
						{
							if (!option_universe.empty() && option_universe.find(opts[i]) == option_universe.end())
								continue;

							enabled = (opts[i] == option);
						}
					}
				}
				else if (c.call.fn == "isEmpty" && c.call.args.size() == 1)
				{
					auto var = vars.find(c.call.args[0]);
					enabled = var == vars.end() || var->second.empty();
				}
				else if (c.call.fn == "contains" && c.call.args.size() == 2)
				{
					auto const * v = this->get(c.call.args[0]);
					enabled = v && std::find(v->begin(), v->end(), c.call.args[1]) != v->end();
				}
				else if (c.call.fn == "infile" && c.call.args.size() == 3)
				{
					env_t nested_env = process_root_qmake_file(fs::absolute(c.call.args[0], this->get_var("PWD")).string());
					enabled = nested_env.get_var(c.call.args[1]) == c.call.args[2];
				}
				else if (c.call.fn == "exists" && c.call.args.size() == 1)
				{
					enabled = fs::exists(fs::absolute(c.call.args[0], this->get_var("PWD")));
				}
				else
				{
					throw std::runtime_error("Unknown function call: " + c.call.fn);
				}
			}
		}

		return enabled;
	}

	std::map<std::string, std::vector<std::string> > vars;
};

void create_msvc_project(env_t const & env, std::string const & proj_file);

#endif // ENV_HPP
