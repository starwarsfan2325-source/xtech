#include "state_w.hpp"
#include "../console.hpp"
#include "../emulator.hpp"
#include "../event.hpp"
#include <string.h>
#include <algorithm>

using namespace ImGui;

namespace xpp::xgui {

namespace {

char notebuf[64];

// commands

void
cmd_savestate(int argc, std::vector<std::string> argv)
{
	state_w *w = (state_w *)get_window("State Manager");
	if (w == NULL) {
		printx(CONSOLE_ERROR, "Couldn't get State Manager window");
		return;
	}
	if (argc > 1) {
		w->new_savestate(argv[1]);
	} else {
		w->new_savestate();
	}
}

void
cmd_loadstate(int argc, std::vector<std::string> argv)
{
	MIN_ARGC(1);
	state_w *w = (state_w *)get_window("State Manager");
	if (w == NULL) {
		printx(CONSOLE_ERROR, "Couldn't get State Manager window");
		return;
	}
	int state_id;
	try {
		state_id = std::stoi(argv[1]);
	} catch (...) {
		printc("Invalid state ID: %s", argv[1].c_str());
		return;
	}
	if (state_id >= 0 && state_id < w->save_states.size())
		w->load_savestate(state_id);
	else
		printc("State ID out of range: %s", argv[1].c_str());
}

void
cmd_rmstate(int argc, std::vector<std::string> argv)
{
	MIN_ARGC(1);
	state_w *w = (state_w *)get_window("State Manager");
	if (w == NULL) {
		printx(CONSOLE_ERROR, "Couldn't get State Manager window");
		return;
	}
	int state_id;
	try {
		state_id = std::stoi(argv[1]);
	} catch (...) {
		printc("Invalid state ID: %s", argv[1].c_str());
		return;
	}
	if (state_id >= 0 && state_id < w->save_states.size()) {
		free(w->save_states[state_id].data);
		w->save_states.erase(w->save_states.begin() + state_id);
	} else {
		printc("State ID out of range: %s", argv[1].c_str());
	}
}

void
cmd_savestatefile(int argc, std::vector<std::string> argv)
{
	MIN_ARGC(1);
	state_w *w = (state_w *)get_window("State Manager");
	if (w == NULL) {
		printx(CONSOLE_ERROR, "Couldn't get State Manager window");
		return;
	}
	w->export_savestates(argv[1]);
}

void
cmd_loadstatefile(int argc, std::vector<std::string> argv)
{
	MIN_ARGC(1);
	state_w *w = (state_w *)get_window("State Manager");
	if (w == NULL) {
		printx(CONSOLE_ERROR, "Couldn't get State Manager window");
		return;
	}
	w->import_savestates(argv[1]);
}

} // namespace

// interface

std::vector<save_state_t> save_states;

state_w::state_w(void) : window_t("State Manager")
{
	/*concmd_new("savestate", cmd_savestate);
	concmd_new("loadstate", cmd_loadstate);
	concmd_new("rmstate", cmd_rmstate);
	concmd_new("savestatefile", cmd_savestatefile);
	concmd_new("loadstatefile", cmd_loadstatefile);*/
	event::subscribe<std::string>("sfc.load_rom", [this](std::string rom) {
		clear_savestates();
		new_savestate("Reset");
	});
	notebuf[0] = '\0';
}

state_w::~state_w(void)
{
}

void
state_w::draw_manager(void)
{
	if (emulator::rom == "empty") {
		Text("Unavailable");
		return;
	}

	int iterator = 0;
	if (Button("New")) {
		printc("Saving state #%d", save_states.size());
		new_savestate();
	}
	SameLine();
	if (Button("Load") && selected_state_id >= 0 && selected_state_id < save_states.size()) {
		printc("Loading state #%d", selected_state_id);
		load_savestate(selected_state_id);
	}
	SameLine();
	if (Button("Remove") && selected_state_id >= 0 && selected_state_id < save_states.size()) {
		printc("Removing state #%d", selected_state_id);
		free(save_states[selected_state_id].data);
		save_states.erase(save_states.begin() + selected_state_id);
	}

	bool state_node = TreeNodeEx("Watches", ImGuiTreeNodeFlags_Leaf, "");
	if (state_node) {
		Columns(2, "SaveStateColumns", false);
		Text("ID"); NextColumn();
		Text("Notes"); HelpMarker("Right-click to edit"); NextColumn();
		Separator();

		SetColumnWidth(0, 67);
		for (auto &i : save_states) {
			std::string popup_id("_sstate_" + std::to_string(iterator));

			
			if (Selectable(std::to_string(iterator).c_str(), iterator == selected_state_id,
				ImGuiSelectableFlags_SpanAllColumns)) {

				selected_state_id = iterator;
			}

			NextColumn();

			Text("%s", i.notes.c_str());
			if (BeginPopupContextItem(popup_id.c_str())) {
				strcpy(notebuf, i.notes.c_str());
				InputText("", notebuf, IM_ARRAYSIZE(notebuf));
				i.notes = std::string(notebuf);
				if (i.notes == "")
					i.notes = " "; // still clickable
				EndPopup();
			}

			NextColumn();
			++iterator;
		}

		Columns(1);
		Separator();
		TreePop();
	}
}

void
state_w::draw(void)
{
	// TODO: rewind
	if (Begin(ICON_FA_MAGIC " State Manager", NULL, ImGuiWindowFlags_MenuBar))
	if (BeginMenuBar()) {
		if (BeginMenu("File")) {
			if (MenuItem("Save States")) export_savestates(emulator::rom + ".state");
			if (MenuItem("Load States")) import_savestates(emulator::rom + ".state");
			EndMenu();
		}
		EndMenuBar();
	}
	draw_manager();
	End();
}

void
state_w::new_savestate(std::string notes)
{
	save_state_t state;
	state.size = emulator::get_state_size();
	state.data = calloc(1, state.size);
	state.notes = notes;
	emulator::save_state(state.data, state.size);
	save_states.push_back(state);
}

void
state_w::load_savestate(int state)
{
	emulator::load_state(save_states[state].data, save_states[state].size);
	event::fire<int>("sfc.load_state", state);
}

void
state_w::export_savestates(std::string filename)
{
	std::ofstream ofs;
	unsigned char note_size;

	ofs.open(filename, std::ios::binary);
	if (!ofs.is_open()) {
		printc("Failed to open %s", filename.c_str());
		return;
	}

	note_size = save_states.size(); // # of states
	ofs.write(&note_size, 1);
	for (auto i : save_states) {
		note_size = strlen(i.notes.c_str());
		ofs.write(&note_size, 1);
		ofs.write(i.notes.c_str(), note_size); // no \0
		ofs.write(i.data, i.size);
	}
	printc("Saved states to %s", filename.c_str());
	ofs.flush();
	ofs.close();
}

void
state_w::import_savestates(std::string filename)
{
	std::ifstream ifs;
	std::string line;
	size_t len, chunk_size;
	unsigned char note_size;
	unsigned char n_states;
	static char buf[257];

	ifs.open(filename);
	if (!ifs.is_open()) {
		printc("Failed to open %s", filename.c_str());
		return;
	}
	ifs.seekg(0, ifs.end);
	len = ifs.tellg();
	ifs.seekg(0, ifs.beg);
	chunk_size = emulator::get_state_size();

	printc("Loading states from %s", filename.c_str());
	save_states.clear(); // TODO: don't clear for empty file
	ifs.read(&n_states, 1);
	for (int i = 0; i < n_states; ++i) {
		save_state_t state;
		ifs.read(&note_size, 1);
		ifs.read(buf, note_size);
		buf[note_size] = '\0';
		state.notes = std::string(buf);
		state.size = chunk_size;
		state.data = calloc(1, chunk_size);
		ifs.read(state.data, chunk_size);
		save_states.push_back(state);
	}
}

void
state_w::clear_savestates(void)
{
	for (int i = 0; i < save_states.size(); ++i)
		free(save_states[i].data);

	save_states.clear();
}

} // namespace
