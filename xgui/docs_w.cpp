#include "docs_w.hpp"
#include "imgui_markdown.h"
#include <fstream>

using namespace ImGui;

namespace xpp::xgui {

namespace {

MarkdownConfig markdown_cfg;

} // namespace

docs_w::docs_w(void) : window_t("Documentation")
{
	under_menu = "Help";
	selected_doc = "";
	directory = "docs";
	body_ptr = NULL;
	editbuf = (char *)calloc(1, 16384);
	markdown_cfg.linkIcon = ICON_FA_LINK;
	refresh_docs(directory);
	markdown_cfg.headingFormats[0].font = xgui::font_h1;
	markdown_cfg.headingFormats[1].font = xgui::font_h2;
	markdown_cfg.headingFormats[2].font = xgui::font_h3;
}

docs_w::~docs_w(void)
{
}

void
docs_w::refresh_docs(std::filesystem::path dir)
{
	if (!std::filesystem::exists(dir))
		return;

	docs.clear();
	for (const auto &i : std::filesystem::directory_iterator(dir)) {
		if (i.is_directory() || (i.path().extension() != ".md" && i.path().extension() != ".txt"))
			continue;

		doc_t doc;
		doc.path = i.path();
		docs.push_back(doc);
	}
}

void
docs_w::load_doc(doc_t &doc)
{
	if (!std::filesystem::exists(doc.path))
		return;

	std::ifstream f(doc.path);
	if (f.is_open()) {
		std::stringstream ss;
		ss << f.rdbuf();
		f.close();
		doc.body = std::move(ss.str());
		body_ptr = &doc.body;
	}
}

void
docs_w::draw(void)
{
	Begin(ICON_FA_BOOK " Documentation", &visible);

	BeginChild("docs_file_pane", ImVec2(GetWindowWidth() * 0.275, 0), true);

	for (auto &i : docs) {
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		if (i.path == selected_doc)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool open = TreeNodeEx(i.path.string().c_str(), flags, i.path.filename().string().c_str());
		if (IsItemClicked()) {
			selected_doc = i.path;
			if (!i.touched)
				load_doc(i);
			if (body_ptr != NULL) {
				size_t n = (*body_ptr).length() >= 16383 ? 16382 : (*body_ptr).length() + 1;
				strncpy(editbuf, (*body_ptr).c_str(), n);
				editbuf[n] = '\0';
			}
		}
		if (open) {
			TreePop();
		}
	}
	EndChild();

	SameLine();

	BeginGroup();
	BeginTabBar("docs_tabs");

	if (BeginTabItem("View")) {
		BeginChild("docs_view");
		if (body_ptr != NULL)
			Markdown(editbuf, strlen(editbuf), markdown_cfg);
		//if (bgfx::isValid(_t))
			//Image(_t, ImVec2(530, 370));
		EndChild();
		EndTabItem();
	}

	if (BeginTabItem("Edit")) {
		BeginChild("docs_edit");
		if (body_ptr != NULL) {
			auto flags = ImGuiInputTextFlags_AllowTabInput;
			InputTextMultiline("##docedit", editbuf, 16383, ImVec2(-FLT_MIN, -FLT_MIN), flags);
		}
		EndChild();
		EndTabItem();
	}

	EndTabBar();
	EndGroup();

	End();
}

} // namespace
