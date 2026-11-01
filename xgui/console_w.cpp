#include "console_w.hpp"
#include "../console.hpp"

using namespace ImGui;

namespace xpp::xgui {

console_w::console_w(void) : window_t("Console")
{
	cmdbuf[0] = '\0';
}

console_w::~console_w(void)
{
}

void
console_w::draw(void)
{
	Begin(ICON_FA_TERMINAL " Console", &visible);

	filter.Draw("Look", GetWindowWidth() - 50.f);

	BeginChildFrame(8192,
		ImVec2(GetWindowWidth() - 16,
		(GetWindowHeight()) - 100), ImGuiWindowFlags_HorizontalScrollbar);
	PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	int n = 0;
	for (auto i : xpp::console::lines) {
		if (!filter.PassFilter(i.string.c_str()))
			continue;
		switch (i.level) {
		case 0:
		default:
			TextWrapped("%s", i.string.c_str());
			break;
		case CONSOLE_WARN:
			PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .85f, .3f, 1.f));
			TextWrapped("[WARN] %s", i.string.c_str());
			PopStyleColor();
			break;
		case CONSOLE_ERROR:
			PushStyleColor(ImGuiCol_Text, ImVec4(1.f, .3f, .3f, 1.f));
			TextWrapped("[ERROR] %s", i.string.c_str());
			PopStyleColor();
			break;
		case CONSOLE_LINK:
			PushStyleColor(ImGuiCol_Text, ImVec4(.3f, 1.f, 1.f, 1.f));
			TextWrapped("%s", i.string.c_str());
			PopStyleColor();
			SameLine();
			//if (SmallButton(stringf(":%d##%d", i.number, n)))
				//lua_debugger::goto_error(i.string, i.number);
			break;
		}
		++n;
	}
	PopStyleVar();
	if (GetScrollY() >= GetScrollMaxY())
		SetScrollHereY(1.0f);
	EndChildFrame();

	Separator();

	bool reclaim_focus = false;
	//PushItemWidth(GetWindowWidth() - 112);
	PushItemWidth(GetWindowWidth() - 112 - 40);
	if (InputText("##_CMDBUF", cmdbuf, IM_ARRAYSIZE(cmdbuf),
		ImGuiInputTextFlags_EnterReturnsTrue //| ImGuiInputTextFlags_CallbackHistory
		)) {

		xpp::console::send(cmdbuf);
		cmdbuf[0] = '\0';
		reclaim_focus = true;
	}
	SetItemDefaultFocus();
	if (reclaim_focus)
		SetKeyboardFocusHere(-1);

	PopItemWidth();

	SameLine();
	if (Button("Send") && cmdbuf[0] != '\0') {
		xpp::console::send(cmdbuf);
		cmdbuf[0] = '\0';
		// TODO
		reclaim_focus = true;
	}
	SameLine();
	if (Button("Lua") && cmdbuf[0] != '\0') {
		printc("> %s", cmdbuf);
		try {
			lua.safe_script(cmdbuf);
		} catch (const sol::error &e) {
			printx(CONSOLE_ERROR, "%s", e.what());
		}
		cmdbuf[0] = '\0';
		// TODO
		reclaim_focus = true;
	}
	SameLine();
	if (Button("Clear"))
		xpp::console::clear();

	End();
}

} // namespace
