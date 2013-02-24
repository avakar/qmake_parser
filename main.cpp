#include "qmake.hpp"
#include <fstream>
#include <stdexcept>
#include <map>
#include <iostream>

class env_t;

void process_qmake_file(std::string const & fname, env_t & env);

struct process_context
{
	process_context(std::string const & filename)
		: filename(filename)
	{
		size_t pos = filename.find_last_of("/\\");
		dir = pos == std::string::npos? "": filename.substr(0, pos);
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

	void process_var_stmt(process_context & ctx, var_stmt const & s)
	{
		std::vector<std::string> processed_values;
		for (size_t i = 0; i < s.ident_list.size(); ++i)
			processed_values.push_back(translate_value(ctx, s.ident_list[i]));

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
			process_qmake_file(ctx.dir + "\\" + b.call.args[0], *this);
		}
		else
		{
			throw std::runtime_error("Unkonwn function call: " + b.call.fn);
		}
	}

private:
	static std::string translate_value(process_context & ctx, std::string const & s)
	{
		std::string res = s;

		size_t start = 0;
		for (;;)
		{
			size_t pos = res.find("$$PWD", start);
			if (pos == std::string::npos)
				break;

			res.replace(res.begin() + pos, res.begin() + pos + 5, ctx.dir);
			start = pos;
		}

		return res;
	}

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

void process_qmake_file(std::string const & fname, env_t & env)
{
	std::filebuf fin;
	if (!fin.open(fname, std::ios::in))
		throw std::runtime_error("Cannot open file: " + fname);

	parser p;
	for (;;)
	{
		char buf[1024];
		std::streamsize read = fin.sgetn(buf, 1024);
		if (read == 0)
			break;
		p.push_data(buf, buf + read);
	}

	std::shared_ptr<block_stmt> stmts = p.finish();

	process_context ctx(fname);
	env.process_block_stmt(ctx, stmts.get());
}

int main(int argc, char * argv[])
{
	env_t env;
	env.add_var("CONFIG", "debug");
	env.add_var("CONFIG", "win32");
	env.add_var("CONFIG", "win32-msvc*");

	try
	{
		process_qmake_file(argv[1], env);

		for (auto it = env.begin(); it != env.end(); ++it)
		{
			std::cout << it->first << ":" << std::endl;
			for (size_t i = 0; i < it->second.size(); ++i)
			{
				std::cout << "    " << it->second[i] << std::endl;
			}
		}
	}
	catch (std::exception const & e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}
