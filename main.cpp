#include "qmake.hpp"
#include <fstream>
#include <stdexcept>
#include <map>
#include <iostream>
#include "env.hpp"

void print_vars(env_t const & env)
{
	for (auto it = env.begin(); it != env.end(); ++it)
	{
		std::cout << it->first << ":" << std::endl;
		for (size_t i = 0; i < it->second.size(); ++i)
		{
			std::cout << "    " << it->second[i] << std::endl;
		}
	}
}

bool process_qmake_file(std::string const & fname, env_t & env)
{
	std::string old_pwd = env.get_var("PWD");

	size_t pos = fname.find_last_of("/\\");
	std::string dir = pos == std::string::npos? "": fname.substr(0, pos);
	env.set_var("PWD", dir);

	std::filebuf fin;
	if (!fin.open(fname, std::ios::in))
		return false;

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

	env.set_var("PWD", old_pwd);
	return true;
}

env_t process_root_qmake_file(std::string const & fname)
{
	env_t env;
	env.add_var("CONFIG", "debug");
	env.add_var("CONFIG", "win32");
	env.add_var("CONFIG", "win32-msvc*");

	env.set_var("ROOT_FILE", fname);
	env.add_var("ROOT_DIR", fs::path(fname).remove_filename().string());

	process_qmake_file(fname, env);
	return env;
}

void make_project(env_t const & env)
{
	std::string const & templ = env.get_one("TEMPLATE");
	if (templ == "subdirs")
	{
		auto const & subdirs = env.get_many("SUBDIRS");

		// TODO: toposort subdirs by .depends

		for (size_t i = 0; i < subdirs.size(); ++i)
		{
			std::string const & subdir = subdirs[i];

			env_t nested_env = process_root_qmake_file(fs::absolute(fs::path(subdir) / (subdir + ".pro"), env.get_one("ROOT_DIR")).string());
			make_project(nested_env);
		}
	}
	else if (templ == "lib")
	{
		// XXX
	}
	else if (templ == "app")
	{
		//print_vars(env);

		create_msvc_project(env, fs::path(env.get_var("ROOT_FILE")).replace_extension(".vcxproj").string());
	}
	else
	{
		throw std::runtime_error("Unknown template: " + templ);
	}
}

void make_solution(env_t const & env)
{
	make_project(env);
}

int main(int argc, char * argv[])
{
	srand(time(0));

	try
	{
		env_t env = process_root_qmake_file(argv[1]);
		make_solution(env);
		//print_vars(env);
	}
	catch (std::exception const & e)
	{
		std::cout << e.what() << std::endl;
		return 1;
	}
}
