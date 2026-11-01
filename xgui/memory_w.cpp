#include "memory_w.hpp"
#include "../console.hpp"
#include "../emulator.hpp"
#include "imgui_memory_editor.h"
#include <dlfcn.h>
#include <fstream>
#include <algorithm>

using namespace ImGui;

namespace xpp::xgui {

namespace {

MemoryEditor mem_edit; // TODO 2 memory editors?
MemoryEditor mem_edit_object;

unsigned char *wram, *vram;
size_t wram_size, vram_size;

bool hex_addresses = true;
bool hex_values = false;

char evalbuf[100];
char notebuf[64];

std::vector<memory_search_t> memory_searches;
unsigned int search_value = 0;
SEARCH_CMP search_compare = CMP_EQ;

bool
pass_search_filter(unsigned int value)
{
	switch (search_compare) {
	case CMP_EQ:
		return (value == search_value);
		break;
	case CMP_NOT:
		return (value != search_value);
		break;
	case CMP_LT:
		return (value < search_value);
		break;
	case CMP_GT:
		return (value > search_value);
		break;
	case CMP_LE:
		return (value <= search_value);
		break;
	case CMP_GE:
		return (value >= search_value);
		break;
	default:
		printf("Invalid search comparison %d\n", search_compare);
		exit(68);
		break;
	}
	return false;
}

bool
pass_search_filter_erase(const memory_search_t &i)
{
	return !pass_search_filter(i.value);
}

/*static bool
pass_search_filter_lua(const memory_search_t &i)
{
	bool tmp;

	// TODO: don't push and pop every time?
	lua_getglobal(debugL, "memfilter");
	lua_pushnumber(debugL, i.value);
	lua_pushnumber(debugL, i.last);
	zlua_call(debugL, 2, 1);
	tmp = !((bool)(lua_toboolean(debugL, -1)));
	lua_pop(debugL, 1);
	return tmp;
}*/

/*static bool
pass_search_filter_lua_preview(unsigned int value, unsigned int last)
{
	bool tmp;

	// TODO: don't push and pop every time?
	lua_getglobal(debugL, "memfilter");
	lua_pushnumber(debugL, value);
	lua_pushnumber(debugL, last);
	zlua_call(debugL, 2, 1);
	tmp = (bool)(lua_toboolean(debugL, -1));
	lua_pop(debugL, 1);
	return tmp;
}*/

} // namespace

// interface

bool
memory_w::memory_watch_has_address(size_t address)
{
	for (auto i : memory_watches) {
		if (address == i.address) {
			return true;
		}
	}
	return false;
}

bool
memory_w::memory_watch_push(size_t address, std::string notes)
{
	if (memory_watch_has_address(address)) {
		printc("Already watching $%06X", address);
		return false;
	}
	memory_watch_t w = {address, 0, 0, WATCH_U8, notes};
	memory_watches.push_back(w);
	return true;
}

void
memory_w::memory_search_push(size_t address)
{
	memory_search_t w = {address, 0, 0};
	memory_searches.push_back(w);
}

/*void
cmd_watch(int argc, std::vector<std::string> argv)
{
	MIN_ARGC(1);
	size_t address = 0;
	if (argv[1].length() < 1)
		return;

	if (argv[1][0] == '$')
		argv[1] = std::string("0x" + argv[1].substr(1));
	if (argv[1].length() > 1 && argv[1][0] == '0' && argv[1][1] == 'x') {
		// hex address
		std::stringstream ss;
		ss << std::hex << argv[1];
		ss >> address;
	} else {
		// decimal address
		try {
			address = std::stoi(argv[1]);
		} catch (...) {
			printc("Invalid address: %s", argv[1].c_str());
			return;
		}
	}

	if (address >= wram_size) {
		printc("Address too large: $%06X > $%06X", address, wram_size);
		return;
	}

	if (argc > 2) {
		if (memory_watch_push(address, argv[2]))
			printc("Watching $%06X (%s)", address, argv[2].c_str());
	} else {
		if (memory_watch_push(address, "..."))
			printc("Watching $%06X", address);
	}
}*/

void
memory_w::save_watches(std::string filename)
{
	std::ofstream ofs;
	ofs.open(filename);

	if (!ofs.is_open()) {
		printc("Failed to open %s", filename.c_str());
		return;
	}

	for (auto i : memory_watches) {
		ofs << "watch " << i.address << " " << i.notes << std::endl;
	}
	printc("Saved watches to %s", filename.c_str());
	ofs.flush();
	ofs.close();
}

void
memory_w::load_watches(std::string filename)
{
	std::ifstream ifs(filename);
	std::string line;

	if (!ifs.is_open()) {
		printc("Failed to open %s", filename.c_str());
		return;
	}

	printc("Loading watches from %s", filename.c_str());
	memory_watches.clear(); // TODO: don't clear for empty file
	while (std::getline(ifs, line)) {
		console::send(line, '>');
	}
}

void
memory_w::update_watches(void)
{
	for (auto &i : memory_watches) {
		i.last = i.value;
		/* casts don't work for signed/unsigned TODO FIXME */
		switch (i.type) {
		case WATCH_U8:
			i.value = (*((uint8_t *)wram + i.address));
			break;
		case WATCH_U16:
			i.value = (*((uint16_t *)wram + i.address));
			break;
		case WATCH_16:
			i.value = (*((int16_t *)wram + i.address));
			break;
		default:
			printf("Unknown WATCH data type %d\n", i.value);
			exit(69);
			break;
		}
	}
}

void
memory_w::update_search(void)
{
	for (auto &i : memory_searches) {
		i.last = i.value;
		i.value = (*((uint8_t *)wram + i.address));
	}
}

void
memory_w::filter_addresses(void)
{
	memory_searches.erase(std::remove_if(memory_searches.begin(),
		memory_searches.end(), pass_search_filter_erase), memory_searches.end());
}

/*static void
memory_w::filter_addresses_lua(void)
{
	memory_searches.erase(std::remove_if(memory_searches.begin(),
		memory_searches.end(), pass_search_filter_lua), memory_searches.end());
}*/

void
memory_w::reset_search(void)
{
	memory_searches.clear();
	for (int i = 0; i < wram_size; ++i)
		memory_search_push(i);
}

memory_w::memory_w(void) : window_t("Memory")
{
	vram = (uint8_t *)emulator::get_vram();
	vram_size = emulator::get_vram_size();
	wram = (uint8_t *)emulator::get_ram();
	wram_size = emulator::get_ram_size();
	mem_edit_object.OptShowDataPreview = true;
	mem_edit_object.PreviewDataType = (MemoryEditor::DataType)ImGuiDataType_U8;
	reset_search();

	//concmd_new("watch", cmd_watch);
	evalbuf[0] = '\0';
	notebuf[0] = '\0';
}

void
memory_w::draw_watch(void)
{
	Checkbox("Hexadecimal addresses", &hex_addresses);
	SameLine();
	Checkbox("Hexadecimal values", &hex_values);
	bool watches_node = TreeNodeEx("Watches", ImGuiTreeNodeFlags_Leaf, "");
	SameLine();
	//HelpMarker("Addresses and values can be right-clicked");
	if (watches_node) {
		Columns(4, "WatchColumns", false);
		Text("Address"); HelpMarker("wram Offset"); NextColumn();
		Text("Value"); NextColumn();
		Text("Last"); NextColumn();
		Text("Notes"); NextColumn();
		Separator();

		for (auto &i : memory_watches) {
			std::string popup_id1("_v1_" + std::to_string(i.address));
			std::string popup_id2("_v2_" + std::to_string(i.address));
			std::string popup_id3("_v3_" + std::to_string(i.address));

			if (hex_addresses)
				Text("$%06lX", i.address);
			else
				Text("%ld", i.address);
			if (BeginPopupContextItem(popup_id1.c_str())) {
				if (Selectable("W.I.P.")) {}
				EndPopup();
			}

			NextColumn();

			if (hex_values) {
				if (i.type == WATCH_U8)
					Text("$%02X", i.value);
				else
					Text("$%04X", i.value);
			} else {
				Text("%d", i.value);
			}
			if (BeginPopupContextItem(popup_id2.c_str())) {
				if (MenuItem("8-bit unsigned", NULL, i.type == WATCH_U8))
					i.type = WATCH_U8;
				if (MenuItem("16-bit unsigned", NULL, i.type == WATCH_U16))
					i.type = WATCH_U16;
				if (MenuItem("16-bit signed", NULL, i.type == WATCH_16))
					i.type = WATCH_16;
				EndPopup();
			}

			NextColumn();

			if (hex_values) {
				if (i.type == WATCH_U8)
					Text("$%02X", i.last);
				else
					Text("$%04X", i.last);
			} else {
				Text("%d", i.last);
			}

			NextColumn();

			Text("%s", i.notes.c_str());
			if (BeginPopupContextItem(popup_id3.c_str())) {
				strcpy(notebuf, i.notes.c_str());
				InputText("", notebuf, IM_ARRAYSIZE(notebuf));
				i.notes = std::string(notebuf);
				if (i.notes == "")
					i.notes = " "; // still clickable
				EndPopup();
			}

			NextColumn();
		}

		Columns(1);
		Separator();
		TreePop();
	}
}

void
memory_w::draw_search(void)
{
	int iterations = 0;
	ImVec4 red(1.f, 0.5f, 0.5f, 1.f);
	ImVec4 white(1.f, 1.f, 1.f, 1.f);
	BeginChild("left pane", ImVec2(256, 0), true);

	bool search_node = TreeNodeEx("search_node", ImGuiTreeNodeFlags_Leaf,
		"%ld addresses", memory_searches.size());

	HelpMarker("Left-click: watch\nRight-click: poke");

	if (search_node) {
		Columns(3, "SearchColumns", false);
		Text("Address"); NextColumn();
		Text("Value"); NextColumn();
		Text("Last"); NextColumn();
		Separator();

		static char _addrbuf[16];

		//lua_getglobal(debugL, "memfilter");
		for (auto &i : memory_searches) {
			//if(lua_isfunction (debugL, -1)) {
			if (false) {
				/*if (!pass_search_filter_lua_preview(i.value, i.last))
					PushStyleColor(ImGuiCol_Text, red);
				else if (memory_watch_has_address(i.address))
					PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.82f, 0.05f, 1.00f));
				else
					PushStyleColor(ImGuiCol_Text, white);*/
			} else {
				if (memory_watch_has_address(i.address))
					PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.82f, 0.05f, 1.00f));
				else
					PushStyleColor(ImGuiCol_Text, white);
			}

			if (hex_addresses)
				snprintf(_addrbuf, sizeof(_addrbuf), "$%06lX", i.address);
			else
				snprintf(_addrbuf, sizeof(_addrbuf), "%ld", i.address);

			if (Selectable(_addrbuf, false, ImGuiSelectableFlags_SpanAllColumns)) {
				console::send(std::string("watch ") + std::to_string(i.address));
			}

			// TODO: poke and freeze
			/*static std::string popup_id;
			popup_id = std::string("_search_") + std::to_string(i.address);
			if (BeginPopupContextItem(popup_id.c_str())) {
				if (Selectable("Test")) {}
				EndPopup();
			}*/

			NextColumn();

			Text("%d", i.value);

			NextColumn();

			Text("%d", i.last);

			NextColumn();

			++iterations;
			if (iterations > 256) {
				PushStyleColor(ImGuiCol_Text, white);
				TextColored(white, "+%ld", memory_searches.size() - iterations);
				NextColumn();
				TextColored(white, "...");
				NextColumn();
				TextColored(white, "...");
				NextColumn();
				PopStyleColor();
				PopStyleColor();
				break;
			}
			PopStyleColor();
		}
		//lua_pop(debugL, 1);

		Columns(1);
		Separator();
		TreePop();
	}

	EndChild();
	SameLine();

	BeginGroup();

	// TODO: typeable lua script run instead, evaluate return value
	Text("Lua filter");
	HelpMarker("Return true to keep address\nReturn false to filter out");

	PushItemWidth(GetWindowWidth() - 256 - 20);
	if (InputTextWithHint("cmpluaeval", "function(value, last)", evalbuf, IM_ARRAYSIZE(evalbuf),
		ImGuiInputTextFlags_EnterReturnsTrue)) {

		// temporary in case of error
		std::string evalstr("function memfilter2(value, last) if 1 then ");
		evalstr.append(evalbuf);
		evalstr.append(" end if 1 then return true end end");
		//if (zlua_dostring(debugL, evalstr.c_str()))
			//zlua_dostring(debugL, "memfilter = memfilter2");
	}

	// TODO fix code duplication
	if (Button("Filter")) {
		if (evalbuf[0] != '\0') {
			// safety first
			std::string evalstr("function memfilter(value, last) if 1 then ");
			evalstr.append(evalbuf);
			evalstr.append(" end if 1 then return true end end");
			/*lua_getglobal(debugL, "memfilter");
			if(lua_isfunction (debugL, -1)) {
				if (zlua_dostring(debugL, evalstr.c_str()))
					filter_addresses_lua();
			}
			lua_pop(debugL, 1);*/
		}
	}
	SameLine();
	if (Button("Preview")) {
		// TODO fix code duplication
		// temporary in case of error
		std::string evalstr("function memfilter2(value, last) if 1 then ");
		evalstr.append(evalbuf);
		evalstr.append(" end if 1 then return true end end");
		//if (zlua_dostring(debugL, evalstr.c_str()))
			//zlua_dostring(debugL, "memfilter = memfilter2");
	}
	SameLine();
	if (Button("Undo")) {
		
	}
	SameLine();
	if (Button("Reset")) {
		reset_search();
	}

	Separator();

	Checkbox("Hexadecimal addresses", &hex_addresses);
	Checkbox("Hexadecimal values", &hex_values);

	EndGroup();
}

static std::string
object_id_to_name(uint8_t id)
{
	return lua["sfc"]["object_names"][2][id];
}

void
memory_w::draw_objects(void)
{
	static int sel = 0;
	RadioButton("Player", &sel, 0);
	SameLine();
	RadioButton("Enemy", &sel, 1);
	uint16_t camx = emulator::read_u16_le(0x1E7A);
	uint16_t camy = emulator::read_u16_le(0x1E7C);
	if (sel == 0) {
		uint16_t px = emulator::read_u16_le(0x9FA);
		uint16_t py = emulator::read_u16_le(0x9FC);
		return;
	}
	uint16_t start = 0xD18;
	uint16_t base = start + (enemy_id * 0x40);
	uint16_t x = 0, y = 0;

	if (BeginCombo("##enemyfollowdropdownmem", object_id_to_name(emulator::read_u8(base + 0x0A)).c_str())) {
		for (int i = 0; i < 32; ++i) {
			base = start + (i * 0x40);
			if (emulator::read_u8(base) == 0 || emulator::read_u8(base + 0x27) == 128)
				continue;
			PushID(i);
			if (Selectable(object_id_to_name(emulator::read_u8(base + 0x0A)).c_str(), enemy_id == i))
				enemy_id = i;
			PopID();
		}
		ImGui::EndCombo();
	}
	base = start + (enemy_id * 0x40);

	if (emulator::read_u8(base) == 0 || emulator::read_u8(base + 0x27) == 128) {
		enemy_id = 0;
		for (int i = 0; i < 32; ++i) {
			base = start + (i * 0x40);
			if (emulator::read_u8(base) != 0 && emulator::read_u8(base + 0x27) != 128) {
				enemy_id = i;
				break;
			}
		}
		return;
	}

	PushFont(font_mono);

	if (BeginTabBar("ObjectMemoryInfoTabs", ImGuiTabBarFlags_None)) {
	if (BeginTabItem("View")) {
		static bool use_hex = false;
		Checkbox("Hexadecimal values", &use_hex);
		std::string fmt = "%d";
		std::string fmt16 = "%d";
		if (use_hex) {
			fmt = "%02X";
			fmt16 = "%04X";
		}
		Separator();
		uint8_t type = emulator::read_u8(base + 0x0A);
		Text(("Type:        "+fmt+" (%s)").c_str(), type, object_id_to_name(type).c_str());
		Text(("Sub-type:    "+fmt).c_str(), emulator::read_u8(base + 0x0B));
		Text(("Health:      "+fmt).c_str(), emulator::read_u8(base + 0x27));
		Separator();
		Text(("Screen X:    "+fmt16).c_str(), -(-emulator::read_u16_le(base + 0x05) - -camx));
		Text(("Screen Y:    "+fmt16).c_str(), (emulator::read_u16_le(base + 0x08) - camy));
		Separator();
		Text(("Sub    X:    "+fmt).c_str(), emulator::read_u8(base + 0x04));
		Text(("Sub    Y:    "+fmt).c_str(), emulator::read_u8(base + 0x07));
		Separator();
		Text(("Speed  X:    "+fmt16).c_str(), emulator::read_u16_le(base + 0x1A));
		Text(("Speed  Y:    "+fmt16).c_str(), emulator::read_u16_le(base + 0x1C));
		Text(("Accel  Y:    "+fmt).c_str(), emulator::read_u8(base + 0x1E));
		Separator();
		Text(("Action 1:    "+fmt).c_str(), emulator::read_u8(base + 0x01));
		Text(("Action 2:    "+fmt).c_str(), emulator::read_u8(base + 0x02));
		Separator();
		Text(("Anim timer:  "+fmt).c_str(), emulator::read_u8(base + 0x13));
		Text(("Sprite no.:  "+fmt).c_str(), emulator::read_u8(base + 0x17));
		Separator();
		EndTabItem();
	}
	if (BeginTabItem("Edit")) {
		uint8_t step8 = 1;
		uint16_t step16 = 1;
		Columns(2, "object_memory_edit_columns", true);
		InputScalar("Type##_otype", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x0A), &step8);
		InputScalar("Health##_ohealth", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x27), &step8);
		InputScalar("X Pos", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x05), &step8);
		InputScalar("Y Pos", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x08), &step8);
		InputScalar("X Speed", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x1A), &step8);
		InputScalar("Y Speed", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x1C), &step8);
		InputScalar("Y Accel", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x1E), &step8);
		InputScalar("Action 1", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x01), &step8);
		InputScalar("Action 2", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x02), &step8);
		InputScalar("Anim timer", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x13), &step8);
		InputScalar("Sprite no.", ImGuiDataType_U8, (uint8_t *)(wram + base + 0x17), &step8);
		Spacing();
		Separator();
		Spacing();
		/*Text(("Type:        "+fmt+" (%s)").c_str(), type, object_id_to_name(type).c_str());
		Text(("Sub-type:    "+fmt).c_str(), emulator::read_u8(base + 0x0B));
		Text(("Health:      "+fmt).c_str(), emulator::read_u8(base + 0x27));
		Separator();
		Text(("Screen X:    "+fmt16).c_str(), -(-emulator::read_u16_le(base + 0x05) - -camx));
		Text(("Screen Y:    "+fmt16).c_str(), (emulator::read_u16_le(base + 0x08) - camy));
		Separator();
		Text(("Sub    X:    "+fmt).c_str(), emulator::read_u8(base + 0x04));
		Text(("Sub    Y:    "+fmt).c_str(), emulator::read_u8(base + 0x07));
		Separator();
		Text(("Speed  X:    "+fmt16).c_str(), emulator::read_u16_le(base + 0x1A));
		Text(("Speed  Y:    "+fmt16).c_str(), emulator::read_u16_le(base + 0x1C));
		Text(("Accel  Y:    "+fmt).c_str(), emulator::read_u8(base + 0x1E));
		Separator();
		Text(("Action 1:    "+fmt).c_str(), emulator::read_u8(base + 0x01));
		Text(("Action 2:    "+fmt).c_str(), emulator::read_u8(base + 0x02));
		Separator();
		Text(("Anim timer:  "+fmt).c_str(), emulator::read_u8(base + 0x13));
		Text(("Sprite no.:  "+fmt).c_str(), emulator::read_u8(base + 0x17));
		Separator();*/
		Columns(1);
		EndTabItem();
	}
	}
	EndTabBar();

	if (BeginTabBar("ObjectMemoryTabs", ImGuiTabBarFlags_None)) {
		if (BeginTabItem("Full memory")) {
			mem_edit_object.Cols = 16;
			mem_edit_object.DrawContents(wram, 64, base);
			EndTabItem();
		}
		if (BeginTabItem("Other memory")) {
			mem_edit_object.Cols = 12;
			mem_edit_object.DrawContents(wram, 24, base + 0x28);
			EndTabItem();
		}
		EndTabBar();
	}

	PopFont();
}

void
memory_w::draw(void)
{
	vram = (uint8_t *)emulator::get_vram();
	vram_size = emulator::get_vram_size();
	wram = (uint8_t *)emulator::get_ram();
	wram_size = emulator::get_ram_size();
	Begin(ICON_FA_MICROCHIP " Memory", &visible, ImGuiWindowFlags_MenuBar);
	if (BeginMenuBar()) {
		if (BeginMenu("File")) {
			if (MenuItem("Save Watches")) {save_watches("build/mm7.watch");}
			if (MenuItem("Load Watches")) {load_watches("build/mm7.watch");}
			EndMenu();
		}
		EndMenuBar();
	}
	if (BeginTabBar("MemoryWatchTabs", ImGuiTabBarFlags_None)) {
		if (BeginTabItem("WRAM")) {
			PushFont(font_mono);
			mem_edit.DrawContents(wram, wram_size, 0x0000);
			PopFont();
			EndTabItem();
		}
		if (BeginTabItem("VRAM")) {
			PushFont(font_mono);
			mem_edit.DrawContents(vram, vram_size, 0x0000);
			PopFont();
			EndTabItem();
		}
		if (BeginTabItem("Watch")) {
			update_watches();
			draw_watch();
			EndTabItem();
		}
		if (BeginTabItem("Search")) {
			update_search();
			draw_search();
			EndTabItem();
		}
		if (BeginTabItem("Objects")) {
			draw_objects();
			EndTabItem();
		}
		EndTabBar();
	}
	End();
}

}
