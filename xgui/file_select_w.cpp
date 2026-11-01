#include "file_select_w.hpp"
#include <fstream>
#include <algorithm>

using namespace ImGui;

namespace xpp::xgui {

file_select_w::file_select_w(std::string title) : window_t(title, false)
{
	name = title;
	directory = std::filesystem::current_path();
	selected_dir = directory;
}

file_select_w::~file_select_w(void)
{
}

void
file_select_w::open(std::string dir, std::string ext, bool reset)
{
	bool do_reset = reset || directory == std::filesystem::current_path();
	if (do_reset) {
		if (dir == ".")
			directory = std::filesystem::current_path();
		else
			directory = std::filesystem::path(dir);
	}

	if (!std::filesystem::exists(directory)) {
		printx(CONSOLE_ERROR, "Couldn't open file select (%s does not exist)");
		return;
	}

	if (do_reset) {
		folders.clear();
		files.clear();
		selected_dir = directory;
		selected_file = "";
	}
	opened_file = "";
	extension = ext;

	if (do_reset) {
		for (const auto &i : std::filesystem::directory_iterator(selected_dir)) {
			if (!i.is_directory())
				continue;

			dir_t _dir;
			_dir.path = i.path();
			folders.push_back(std::move(_dir));
		}
		refresh_files();
	}

	is_open = true;
}

void
file_select_w::draw(void)
{
	if (is_open)
		OpenPopup((ICON_FA_FILE " " + name).c_str());
	else
		return;

	SetNextWindowSize(ImVec2(640, 480));
	if (!BeginPopupModal((ICON_FA_FILE " " + name).c_str(), NULL))
		return;

	filter.Draw("Look", GetWindowWidth() - 50.f);

	Separator();

	std::string str;
	if (selected_dir == "/")
		str = std::filesystem::current_path().append(directory.string());
	else
		str = std::filesystem::current_path().append(selected_dir.string());
	if (selected_file != "" && std::filesystem::exists(selected_file))
		str += "/" + selected_file.filename().string();
	Text("%s", str.c_str());

	BeginChild("file_picker_list", ImVec2(GetWindowWidth() * 0.6, -36), true);
	int n = 0;
	bool double_clicked_file = false;
	for (auto &i : files) {
		if (!filter.PassFilter(i.path().filename().c_str()))
			continue;

		if (Selectable(i.path().filename().c_str(),
			selected_file == i.path(), ImGuiSelectableFlags_AllowDoubleClick)) {

			selected_file = i.path();
			if (IsMouseDoubleClicked(0))
				double_clicked_file = true;
		}

		++n;
	}
	EndChild();

	SameLine();

	BeginChild("file_picker_folders", ImVec2(GetWindowWidth() * 0.4, -36), true);
	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_Leaf |
		ImGuiTreeNodeFlags_FramePadding |
		ImGuiTreeNodeFlags_SpanAvailWidth |
		ImGuiTreeNodeFlags_NoTreePushOnOpen;
	if (directory == selected_dir)
		flags |= ImGuiTreeNodeFlags_Selected;

	TreeNodeEx(directory.string().c_str(), flags, (directory.filename().string() + "/").c_str());
	if (IsItemClicked()) {
		if (directory != selected_dir)
			selected_file = "";
		selected_dir = directory;
		refresh_files();
	}

	for (auto &i : folders)
		recurse(i);
	EndChild();

	bool valid_file = selected_file != ""
		&& std::filesystem::exists(selected_file)
		&& filter.PassFilter(selected_file.filename().c_str());
	if (!valid_file) {
		PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.75f));
	}
	if ((Button("Open", ImVec2(128, 32)) || double_clicked_file) && valid_file) {
		opened_file = selected_file;
		CloseCurrentPopup();
		is_open = false;
	}
	if (!valid_file)
		PopStyleColor(3);
	SameLine();
	if (Button("Cancel", ImVec2(128, 32))) {
		opened_file = "";
		CloseCurrentPopup();
		is_open = false;
	}

	EndPopup();
}

void
file_select_w::recurse(dir_t &dir)
{
	if (!dir.touched)
		refresh_dir(dir);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
	flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
	flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
	if (dir.path == selected_dir)
		flags |= ImGuiTreeNodeFlags_Selected;

	bool open = TreeNodeEx(dir.path.string().c_str(), flags, (dir.path.filename().string() + "/").c_str());
	if (IsItemClicked()) {
		if (dir.path != selected_dir)
			selected_file = "";
		selected_dir = dir.path;
		refresh_files();
	}

	if (open) {
		for (auto &i : dir.children)
			recurse(i);

		TreePop();
	}
}

void
file_select_w::refresh_dir(dir_t &dir)
{
	if (!std::filesystem::exists(dir.path))
		return;

	dir.children.clear();
	for (const auto &i : std::filesystem::directory_iterator(dir.path)) {
		if (!i.is_directory())
			continue;

		dir_t _dir;
		_dir.path = i.path();
		dir.children.push_back(std::move(_dir));
	}

	dir.touched = true;
}

void
file_select_w::refresh_files(void)
{
	if (!std::filesystem::exists(selected_dir))
		return;

	files.clear();
	for (const auto &i : std::filesystem::directory_iterator(selected_dir)) {
		if (i.is_directory() || (extension != "" && i.path().extension() != extension))
			continue;

		files.push_back(i);
	}
}

} // namespace
