#include "env.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

static char const msvc_filters_template[] =
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	"<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
	"  <ItemGroup>\n"
	"$items"
	"  </ItemGroup>\n"
	"</Project>\n";

static char const msvc_template[] =
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	"<Project DefaultTargets=\"Build\" ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n"
	"  <ItemGroup Label=\"ProjectConfigurations\">\n"
	"    <ProjectConfiguration Include=\"Debug|Win32\">\n"
	"      <Configuration>Debug</Configuration>\n"
	"      <Platform>Win32</Platform>\n"
	"    </ProjectConfiguration>\n"
	"    <ProjectConfiguration Include=\"Release|Win32\">\n"
	"      <Configuration>Release</Configuration>\n"
	"      <Platform>Win32</Platform>\n"
	"    </ProjectConfiguration>\n"
	"  </ItemGroup>\n"
	"  <PropertyGroup Label=\"Globals\">\n"
	"    <ProjectGuid>$guid</ProjectGuid>\n"
	"    <Keyword>Win32Proj</Keyword>\n"
	"    <RootNamespace>$project_name</RootNamespace>\n"
	"  </PropertyGroup>\n"
	"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n"
	"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\" Label=\"Configuration\">\n"
	"    <ConfigurationType>Application</ConfigurationType>\n"
	"    <UseDebugLibraries>true</UseDebugLibraries>\n"
	"    <PlatformToolset>v100</PlatformToolset>\n"
	"    <CharacterSet>Unicode</CharacterSet>\n"
	"  </PropertyGroup>\n"
	"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\" Label=\"Configuration\">\n"
	"    <ConfigurationType>Application</ConfigurationType>\n"
	"    <UseDebugLibraries>false</UseDebugLibraries>\n"
	"    <PlatformToolset>v100</PlatformToolset>\n"
	"    <WholeProgramOptimization>true</WholeProgramOptimization>\n"
	"    <CharacterSet>Unicode</CharacterSet>\n"
	"  </PropertyGroup>\n"
	"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n"
	"  <ImportGroup Label=\"ExtensionSettings\">\n"
	"  </ImportGroup>\n"
	"  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n"
	"    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
	"  </ImportGroup>\n"
	"  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n"
	"    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
	"  </ImportGroup>\n"
	"  <PropertyGroup Label=\"UserMacros\" />\n"
	"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n"
	"    <LinkIncremental>true</LinkIncremental>\n"
	"    <OutDir>$out_dir</OutDir>\n"
	"    <IntDir>$int_dir</IntDir>\n"
	"$targetname"
	"  </PropertyGroup>\n"
	"  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n"
	"    <LinkIncremental>false</LinkIncremental>\n"
	"    <OutDir>$out_dir</OutDir>\n"
	"    <IntDir>$int_dir</IntDir>\n"
	"$targetname"
	"  </PropertyGroup>\n"
	"  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|Win32'\">\n"
	"    <ClCompile>\n"
	"      <AdditionalIncludeDirectories>$include_paths</AdditionalIncludeDirectories>\n"
	"$pch"
	"      <WarningLevel>Level3</WarningLevel>\n"
	"      <Optimization>Disabled</Optimization>\n"
	"      <PreprocessorDefinitions>$pp_defs;WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
	"    </ClCompile>\n"
	"    <Link>\n"
	"      <AdditionalDependencies>$debug_libs</AdditionalDependencies>\n"
	"      <AdditionalLibraryDirectories>$lib_paths</AdditionalLibraryDirectories>\n"
	"      <SubSystem>Console</SubSystem>\n"
	"      <GenerateDebugInformation>true</GenerateDebugInformation>\n"
	"    </Link>\n"
	"$qtuisettings"
	"$qtmocsettings"
	"$qtrccsettings"
	"  </ItemDefinitionGroup>\n"
	"  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|Win32'\">\n"
	"    <ClCompile>\n"
	"      <AdditionalIncludeDirectories>$include_paths</AdditionalIncludeDirectories>\n"
	"      <WarningLevel>Level3</WarningLevel>\n"
	"$pch"
	"      <Optimization>MaxSpeed</Optimization>\n"
	"      <FunctionLevelLinking>true</FunctionLevelLinking>\n"
	"      <IntrinsicFunctions>true</IntrinsicFunctions>\n"
	"      <PreprocessorDefinitions>$pp_defs;WIN32;NDEBUG;_CONSOLE;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n"
	"    </ClCompile>\n"
	"    <Link>\n"
	"      <AdditionalDependencies>$release_libs</AdditionalDependencies>\n"
	"      <AdditionalLibraryDirectories>$lib_paths</AdditionalLibraryDirectories>\n"
	"      <SubSystem>Console</SubSystem>\n"
	"      <GenerateDebugInformation>true</GenerateDebugInformation>\n"
	"      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n"
	"      <OptimizeReferences>true</OptimizeReferences>\n"
	"    </Link>\n"
	"$qtuisettings"
	"$qtmocsettings"
	"$qtrccsettings"
	"  </ItemDefinitionGroup>\n"
	"  <ItemGroup>\n"
	"$files"
	"  </ItemGroup>\n"
	"  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n"
	"  <ImportGroup Label=\"ExtensionTargets\">\n"
	"    <Import Project=\"..\\..\\qmake_parser\\props\\QtMoc.targets\" />\n"
	"    <Import Project=\"..\\..\\qmake_parser\\props\\QtRcCompile.targets\" />\n"
	"    <Import Project=\"..\\..\\qmake_parser\\props\\QtTsCompile.targets\" />\n"
	"    <Import Project=\"..\\..\\qmake_parser\\props\\QtUICompile.targets\" />\n"
	"  </ImportGroup>\n"
	"</Project>";

fs::path relative(fs::path const & p, fs::path const & base)
{
	if (p == base)
		return ".";

	if (p.root_name() != base.root_name())
		return p;

	fs::path from_path, from_base, output;

	fs::path::iterator path_it = p.begin(), path_end = p.end();
	fs::path::iterator base_it = base.begin(), base_end = base.end();

	// Cache system-dependent dot, double-dot and slash strings
	const std::string _dot  = ".";
	const std::string _dots = "..";
	const std::string _sep = "/";

	// iterate over path and base
	for (;;)
	{
		// compare all elements so far of path and base to find greatest common root;
		// when elements of path and base differ, or run out:
		if ((path_it == path_end) || (base_it == base_end) || (*path_it != *base_it))
		{
			// write to output, ../ times the number of remaining elements in base;
			// this is how far we've had to come down the tree from base to get to the common root
			for (; base_it != base_end; ++base_it)
			{
				if (*base_it == _dot)
					continue;
				else if (*base_it == _sep)
					continue;

				output /= "../";
			}

			// write to output, the remaining elements in path;
			// this is the path relative from the common root
			boost::filesystem::path::iterator path_it_start = path_it;
			for (; path_it != path_end; ++path_it)
			{
				if (path_it != path_it_start)
					output /= "/";

				if (*path_it == _dot)
					continue;
				if (*path_it == _sep)
					continue;

				output /= *path_it;
			}

			break;
		}

		// add directory level to both paths and continue iteration
		from_path /= fs::path(*path_it);
		from_base /= fs::path(*base_it);

		++path_it, ++base_it;
	}

	return output;
}

fs::path relative(fs::path const & p)
{
	return relative(p, fs::current_path());
}

void add_file_items(std::string const & proj_file_dir, std::vector<std::string> sources, std::string const & tag, std::string & files,
	std::string & filter_items, std::set<std::string> & filters, std::string const & props = "")
{
	std::sort(sources.begin(), sources.end());
	sources.erase(std::unique(sources.begin(), sources.end()), sources.end());
	for (auto it = sources.begin(); it != sources.end(); ++it)
	{
		std::string relpath = relative(*it, proj_file_dir).string();
		boost::algorithm::replace_all(relpath, "/", "\\");

		fs::path p = fs::path(relpath).remove_filename();

		if (props.empty())
		{
			files.append("    <" + tag + " Include=\"" + relpath + "\" />\n");
		}
		else
		{
			files.append("    <" + tag + " Include=\"" + relpath + "\">\n");
			files.append(props);
			files.append("    </" + tag + ">\n");
		}

		filter_items.append("    <" + tag + " Include=\"" + relpath + "\">\n");
		filter_items.append("      <Filter>" + boost::algorithm::replace_all_copy(p.string(), "/", "\\") + "</Filter>\n");
		filter_items.append("    </" + tag + ">\n");

		while (!p.empty())
		{
			filters.insert(boost::algorithm::replace_all_copy(p.string(), "/", "\\"));
			p.remove_filename();
		}
	}
}

void create_msvc_project(env_t const & env, std::string const & proj_file)
{
	fs::path proj_file_dir(proj_file);
	proj_file_dir.remove_filename();

	std::string res = msvc_template;

	std::string files;
	std::string filter_items;
	std::set<std::string> filters;

	std::vector<std::string> var_sources = env.get_var_many("SOURCES");
	std::vector<std::string> var_headers = env.get_var_many("HEADERS");
	std::vector<std::string> includepaths = env.get_var_many("INCLUDEPATH");
	std::vector<std::string> var_resources = env.get_var_many("RESOURCES");

	std::vector<std::string> c_sources, cpp_sources;
	for (size_t i = 0; i < var_sources.size(); ++i)
	{
		if (fs::path(var_sources[i]).extension().string() != ".cpp")
			c_sources.push_back(var_sources[i]);
		else
			cpp_sources.push_back(var_sources[i]);
	}

	{
		std::string moc_dir = env.get_var("MOC_DIR");
		if (!moc_dir.empty())
		{
			moc_dir = relative(moc_dir, proj_file_dir).string();

			std::string defines;

			auto const & defines_var = env.get_var_many("DEFINES");
			for (auto it = defines_var.begin(); it != defines_var.end(); ++it)
			{
				if (!defines.empty())
					defines.push_back(' ');
				defines.append("-D");
				defines.append(*it);
			}

			boost::replace_all(res, "$qtmocsettings",
				"    <QtMoc>\n"
				"      <OutDir>" + moc_dir + "\\</OutDir>\n"
				"      <PreprocessorDefines2>" + defines + " -DWIN32 %(PreprocessorDefines2)</PreprocessorDefines2>\n"
				"    </QtMoc>\n");
			includepaths.push_back(moc_dir);

			for (size_t i = 0; i < var_headers.size(); ++i)
				cpp_sources.push_back(moc_dir + "\\moc_" + fs::path(var_headers[i]).filename().replace_extension().string() + ".cpp");
		}
	}

	{
		std::string rcc_dir = env.get_var("RCC_DIR");
		if (!rcc_dir.empty())
		{
			rcc_dir = relative(rcc_dir, proj_file_dir).string();

			boost::replace_all(res, "$qtrccsettings",
				"    <QtRcCompile>\n"
				"      <OutDir>" + rcc_dir + "\\</OutDir>\n"
				"    </QtRcCompile>\n");

			for (size_t i = 0; i < var_resources.size(); ++i)
				cpp_sources.push_back(rcc_dir + "\\qrc_" + fs::path(var_resources[i]).filename().replace_extension().string() + ".cpp");
		}
	}

	add_file_items(proj_file_dir.string(), cpp_sources, "ClCompile", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), c_sources, "ClCompile", files, filter_items, filters,
		"      <PrecompiledHeader>NotUsing</PrecompiledHeader>\n"
		"      <ForcedIncludeFiles></ForcedIncludeFiles>\n");
	add_file_items(proj_file_dir.string(), var_headers, "QtMoc", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), env.get_var_many("FORMS"), "QtUICompile", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), env.get_var_many("RC_FILE"), "ResourceCompile", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), var_resources, "QtRcCompile", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), env.get_var_many("OTHER_FILES"), "None", files, filter_items, filters);
	add_file_items(proj_file_dir.string(), env.get_var_many("TRANSLATIONS"), "QtTsCompile", files, filter_items, filters);

	std::vector<std::string> lib_paths;
	std::vector<std::string> debug_libs, release_libs;

	{
		auto const & qt = env.get_var_many("QT");
		for (auto it = qt.begin(); it != qt.end(); ++it)
		{
			std::string component = *it;
			if (component.empty())
				continue;

			component[0] = ::toupper(component[0]);
			includepaths.push_back(env.translate_value("$$[QT_INSTALL_HEADER]/Qt" + component));
			debug_libs.push_back("Qt" + component + "d4.lib");
			release_libs.push_back("Qt" + component + "4.lib");
		}
	}

	includepaths.push_back(env.translate_value("$$[QT_INSTALL_HEADER]"));
	lib_paths.push_back(env.translate_value("$$[QT_INSTALL_LIB]"));


	{
		std::string ui_dir = env.get_var("UI_DIR");
		if (!ui_dir.empty())
		{
			ui_dir = relative(ui_dir, proj_file_dir).string();

			boost::replace_all(res, "$qtuisettings",
				"    <QtUICompile>\n"
				"      <OutDir>" + ui_dir + "\\</OutDir>\n"
				"    </QtUICompile>\n");
			includepaths.push_back(ui_dir);
		}
	}

	std::string pch = env.get_var("PRECOMPILED_HEADER");
	if (!pch.empty())
	{
		{
			std::ofstream fout(fs::absolute(pch + ".cpp", proj_file_dir).string());
			fout << "#include \"" + pch + "\"\n";
		}

		files.append(
			"    <ClCompile Include=\"" + pch + ".cpp\">\n"
			"      <PrecompiledHeader>Create</PrecompiledHeader>\n"
			"      <ForcedIncludeFiles></ForcedIncludeFiles>\n"
			"    </ClCompile>\n"
			);

		boost::replace_all(res, "$pch",
			"      <PrecompiledHeader>Use</PrecompiledHeader>\n"
			"      <PrecompiledHeaderFile>" + pch + "</PrecompiledHeaderFile>\n"
			"      <ForcedIncludeFiles>" + pch + "</ForcedIncludeFiles>\n"
			);
	}

	boost::replace_all(res, "$files", files);
	boost::replace_all(res, "$include_paths", boost::algorithm::join(includepaths, ";"));
	boost::replace_all(res, "$debug_libs", boost::algorithm::join(debug_libs, ";"));
	boost::replace_all(res, "$release_libs", boost::algorithm::join(release_libs, ";"));
	boost::replace_all(res, "$lib_paths", boost::algorithm::join(lib_paths, ";"));
	boost::replace_all(res, "$pp_defs", boost::algorithm::join(env.get_many("DEFINES"), ";"));
	boost::replace_all(res, "$project_name", fs::path(env.get_var("ROOT_FILE")).filename().replace_extension().string());
	boost::replace_all(res, "$out_dir", relative(env.get_var("DESTDIR"), proj_file_dir).string());
	boost::replace_all(res, "$int_dir", relative(env.get_var("OBJECTS_DIR"), proj_file_dir).string());

	std::string target = env.get_var("TARGET");
	if (!target.empty())
		boost::replace_all(res, "$targetname", "    <TargetName>" + target + "</TargetName>\n");

	{
		std::string guid = env.get_var("GUID");
		if (guid.empty())
		{
			static char const digits[] = "0123456789ABCDEF";
			guid = "{????????-????-????-????-????????????}";
			for (size_t i = 0; i < guid.size(); ++i)
			{
				if (guid[i] == '?')
					guid[i] = digits[rand() % 16];
			}
		}

		boost::replace_all(res, "$guid", guid);
	}

	std::ofstream fout(proj_file);
	fout << res;
	fout.close();

	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (!it->empty())
			filter_items.append("    <Filter Include=\"" + *it + "\" />\n");
	}

	res = msvc_filters_template;
	boost::replace_all(res, "$items", filter_items);

	std::ofstream ffout(proj_file + ".filters");
	ffout << res;
	ffout.close();
}
