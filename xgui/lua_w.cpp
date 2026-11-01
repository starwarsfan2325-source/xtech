#include "lua_w.hpp"
#include "../console.hpp"
#include "../script.hpp"
#include "TextEditor.h"
#include <iostream>
#include <string>
#include <algorithm>

using namespace ImGui;
using xpp::script::script_t;
using xpp::script::scripts;

namespace xpp::xgui {

namespace {

auto node_flags_table =
	ImGuiTreeNodeFlags_FramePadding |
	ImGuiTreeNodeFlags_SpanFullWidth;
auto node_flags_leaf =
	ImGuiTreeNodeFlags_Leaf |
	ImGuiTreeNodeFlags_FramePadding |
	ImGuiTreeNodeFlags_SpanFullWidth |
	ImGuiTreeNodeFlags_NoTreePushOnOpen;

} // namespace

// interface

lua_w::lua_w(void) : window_t("Lua")
{
	script::load_dir(".", false);
	script::load_dir("obj", false);
}

lua_w::~lua_w(void)
{
}

void
lua_w::draw_lua_code(void)
{
	if (scripts.find(selected_script) == scripts.end())
		return;
	// Leave room for 1 line below us
	BeginGroup();
	BeginChild("script view", ImVec2(0, -GetFrameHeightWithSpacing()));
	AlignTextToFramePadding();
	Text(" %s", scripts[selected_script].filename.c_str());
	SameLine();
	if (Button("Run")) {
		script::run(scripts[selected_script].editor->GetText(), scripts[selected_script].filename);
	}
	SameLine();
	if (Button("Put")) {
		script::save(selected_script);
	}
	Separator();

	PushFont(font_mono);
	scripts[selected_script].editor->Render(scripts[selected_script].filename.c_str());
	PopFont();

	EndChild();
	EndGroup();

	if (scripts[selected_script].editor->IsTextChanged()) {
		scripts[selected_script].modified =
			!(scripts[selected_script].text.c_str() == scripts[selected_script].editor->GetText());
	}
}

void
lua_w::draw_scripts(void)
{
	int n = 0;

	// first main files, then objects
	BeginGroup();

	Text("Core");
	BeginChild("scripts_left_pane_main", ImVec2(150, (GetTextLineHeight() * 16) + 12), true);
	for (std::pair<std::string, script_t> ii : scripts) {
		script_t &i = ii.second;
		if (i.path.size() == 0 || i.path[0] != '.')
			continue;

		bool clicked;
		if (i.modified)
			clicked = Selectable((i.filename + " *").c_str(), selected_script == i.path);
		else
			clicked = Selectable(i.filename.c_str(), selected_script == i.path);

		if (clicked)
			selected_script = i.path; // set_script?

		++n;
	}
	EndChild();

	Text("Objects");
	BeginChild("scripts_left_pane_obj", ImVec2(150, 0), true);
	for (std::pair<std::string, script_t> ii : scripts) {
		script_t &i = ii.second;
		if (i.path.size() < 4 || i.path.substr(0, 4) != "obj/")
			continue;

		bool clicked;
		if (i.modified)
			clicked = Selectable((i.path + " *").c_str(), selected_script == i.path);
		else
			clicked = Selectable(i.path.c_str(), selected_script == i.path);

		if (clicked)
			selected_script = i.path; // set_script?

		++n;
	}
	EndChild();
	EndGroup();

	SameLine();

	if (scripts.size() < 1) {
		Text("No scripts found.");
		return;
	}

	BeginChild("script_code_area");
	BeginGroup();
	if (BeginTabBar("DebuggerScriptsTabs", ImGuiTabBarFlags_None)) {
		if (BeginTabItem("Lua")) {
			draw_lua_code();
			EndTabItem();
		}
		EndTabBar();
	}
	EndGroup();
	EndChild();
}

void
lua_w::draw_watches(void)
{
	
}

void
lua_w::table_recurse(sol::table &parent, sol::object const &key, sol::object &value)
{
	if ((((int)value.get_type())) == LUA_TTABLE) {
		bool node = TreeNodeEx(script::to_string(key).c_str(), node_flags_table);
		NextColumn();
		Text("#%ld", ((sol::table)(value)).size());
		NextColumn();
		Text("table");
		NextColumn();
		if (node) {
			const sol::table &t = const_cast<const sol::table &>((sol::table &)(value));
			((sol::table)(value)).for_each([&t, this]
				(sol::object const &k, sol::object const &v) mutable {
				table_recurse(const_cast<sol::table &>(t), k, const_cast<sol::object &>(v));
			});
			TreePop();
		}
	} else {
		TreeNodeEx(script::to_string(key).c_str(), node_flags_leaf);
		NextColumn();

		if ((((int)value.get_type())) == LUA_TNUMBER) {
			double dbl = value.as<double>();
			double step = 1.0;
			if (InputScalar(("##"+script::to_string(key)+"]"+std::to_string(recurse_inc)).c_str(),
				ImGuiDataType_Double, &dbl, &step)) {

				parent[key] = dbl;
			}
		} else if ((((int)value.get_type())) == LUA_TSTRING) {
			Text("\"%s\"", script::to_string(value).c_str());
		} else {
			Text("%s", script::to_string(value).c_str());
		}
		NextColumn();

		Text("%s", script::type_to_string(value.get_type()).c_str());
		NextColumn();
	}
	++recurse_inc;
}

// TODO: proper table search (can't currently search for table key)

bool
lua_w::table_recurse_test(const int level, sol::table &parent, sol::object const &key, sol::object &value)
{
	if (level > 9)
		return false;

	if ((((int)value.get_type())) == LUA_TTABLE) {
		if (
			value == lua["_G"] ||
			value == lua["package"]
		)
			return false;

		bool found = false;
		const sol::table &t = const_cast<const sol::table &>((sol::table &)(value));
		const bool &b = const_cast<const bool &>((bool &)(found));

		((sol::table)(value)).for_each([&t, &b, &level, this]
			(sol::object const &k, sol::object const &v) mutable {
			if (table_recurse_test(level + 1, const_cast<sol::table &>(t),
				k, const_cast<sol::object &>(v))) {

				const_cast<bool &>(b) = true; // TODO break out?
			}
		});

		return found;
	} else {
		bool pass = filter.PassFilter(script::to_string(key).c_str());
		if (!pass) {
			pass = filter.PassFilter(script::to_string(value).c_str());
			return pass;
		}

		return true;
	}
}

bool
lua_w::table_recurse_look(const int level, sol::table &parent, sol::object const &key, sol::object &value)
{
	if (level > 9)
		return false;

	if ((((int)value.get_type())) == LUA_TTABLE) {
		if (
			value == lua["_G"] ||
			value == lua["package"]
		)
			return false;

		bool show_node = false;
		const sol::table &t = const_cast<const sol::table &>((sol::table &)(value));
		const bool &b = const_cast<const bool &>((bool &)(show_node));

		((sol::table)(value)).for_each([&t, &b, &level, this]
			(sol::object const &k, sol::object const &v) mutable {
			if (table_recurse_test(level + 1, const_cast<sol::table &>(t),
				k, const_cast<sol::object &>(v))) {

				const_cast<bool &>(b) = true;
			}
		});

		if (show_node) {
			bool node = TreeNodeEx(script::to_string(key).c_str(), node_flags_table);
			NextColumn();
			Text("#%ld", ((sol::table)(value)).size());
			NextColumn();
			Text("table");
			NextColumn();
			if (node) {
				((sol::table)(value)).for_each([&t, &b, &level, this]
					(sol::object const &k, sol::object const &v) mutable {
					if (table_recurse_look(level + 1, const_cast<sol::table &>(t),
						k, const_cast<sol::object &>(v))) {

						const_cast<bool &>(b) = true;
					}
				});
				TreePop();
			}
		}

		++recurse_inc;
		return show_node;
	} else {
		bool pass = filter.PassFilter(script::to_string(key).c_str());
		if (!pass) {
			pass = filter.PassFilter(script::to_string(value).c_str());
			return pass;
		}

		TreeNodeEx(script::to_string(key).c_str(), node_flags_leaf);
		NextColumn();

		if ((((int)value.get_type())) == LUA_TNUMBER) {
			double dbl = value.as<double>();
			double step = 1.0;
			if (InputScalar(("##"+script::to_string(key)+"]"+std::to_string(recurse_inc)).c_str(),
				ImGuiDataType_Double, &dbl, &step)) {

				parent[key] = dbl;
			}
		} else if ((((int)value.get_type())) == LUA_TSTRING) {
			Text("\"%s\"", script::to_string(value).c_str());
		} else {
			Text("%s", script::to_string(value).c_str());
		}
		NextColumn();

		Text("%s", script::type_to_string(value.get_type()).c_str());
		NextColumn();

		++recurse_inc;
		return true;
	}

	return false;
}

void
//Todo Search & Egnore & Put in frame for scorll
lua_w::draw_globals(void)
{
	// tree?
	Checkbox("Hexadecimal numbers", &hex_values);
	SameLine();
	Checkbox("Match keys", &match_keys);
	SameLine();
	Checkbox("Match values", &match_values);

	filter.Draw("Look", GetWindowWidth() - 50.f);

	BeginChild("lua_debugger__g");
	Columns(3, "GlobalsColumns", false); // no borders

	float w = GetWindowWidth();
	SetColumnWidth(0, (w / 2.f) - 50);
	SetColumnWidth(1, (w / 2.f) - 50);
	SetColumnWidth(2, 100);
	Text("Key"); NextColumn();
	Text("Value"); NextColumn();
	Text("Type"); NextColumn();
	Separator();

	bool g_node = TreeNodeEx("_G", node_flags_table);
	HelpMarker("Global table");
	NextColumn();
	//Text("#???");
	NextColumn();
	//Text("table");
	NextColumn();
	sol::table _g = lua["_G"];
	if (g_node) {
		recurse_inc = 0;
		sol::table _g = lua["_G"];
		_g.for_each([&_g, this](sol::object const &key, sol::object const &value) {
			table_recurse_look(0, _g, key, const_cast<sol::object &>(value));
		});
		TreePop();
	}

	Columns(1);
	EndChild();

	//Separator();
}

void
lua_w::draw_events(void)
{
	Checkbox("Hide input events", &hide_input_events);

	BeginChild("lua_debugger_events");
	Columns(2, "LuaEventsColumns", false); // no borders

	float w = GetWindowWidth();
	//SetColumnWidth(0, (w / 2.f) - 50);
	//SetColumnWidth(1, (w / 2.f) - 50);
	//SetColumnWidth(2, 100);
	Text("Key"); NextColumn();
	Text("Value"); NextColumn();
	//Text("Type"); NextColumn();
	Separator();

	sol::table journal = lua["event"]["journal"];
	// TODO: full filtering
	if (hide_input_events) {
		journal.for_each([this](sol::object const &key, sol::object const &value) {
			if (value.as<std::string>().rfind("input", 0) != 0) {
				Text("%d", key.as<int>());
				NextColumn();
				Text("%s", value.as<std::string>().c_str());
				NextColumn();
			}
		});
	} else {
		journal.for_each([this](sol::object const &key, sol::object const &value) {
			Text("%d", key.as<int>());
			NextColumn();
			Text("%s", value.as<std::string>().c_str());
			NextColumn();
		});
	}
	

	Columns(1);
	EndChild();
}

void
lua_w::draw(void)
{
	Begin(ICON_FA_CODE " Lua", &visible, ImGuiWindowFlags_MenuBar);

	if (BeginMenuBar()) {
		if (BeginMenu("File")) {
			if (MenuItem("Save")) {}
			EndMenu();
		}
		if (BeginMenu("Run")) {
			if (MenuItem("Reload objects")) {
				sol::protected_function f = lua["x_reload_objects"];
				script::check_result(f(), "main.lua");
			}
			EndMenu();
		}
		EndMenuBar();
	}

	if (BeginTabBar("lua_tabs", ImGuiTabBarFlags_None)) {
		if (BeginTabItem("Scripts")) {
			draw_scripts();
			EndTabItem();
		}
		if (BeginTabItem("State")) {
			draw_globals();
			EndTabItem();
		}
		if (BeginTabItem("Events")) {
			draw_events();
			EndTabItem();
		}
		EndTabBar();
	}

	End();
}

void
lua_w::goto_error(std::string file, int line)
{
	int n = 0;
	for (std::pair<std::string, script_t> ii : scripts) {
		script_t &i = ii.second;
		if (i.filename == file) {
			if (line < 1 || line > i.editor->GetTotalLines()) {
				printc("Line number %d out of range", line);
				return;
			}
			i.editor->SetCursorPosition(TextEditor::Coordinates(line - 1, 0));
			if (n < scripts.size())
				selected_script = n;
			return;
		}
		++n;
	}

	printc("Script %s not found", file.c_str());
}

} // namespace
