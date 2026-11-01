#include "demo_w.hpp"

using namespace ImGui;

namespace xpp::xgui {

demo_w::demo_w(void) : window_t("Demos")
{
}

demo_w::~demo_w(void)
{
}

void
demo_w::draw(void)
{
	Begin(ICON_FA_VIDEO_CAMERA " Demos", &visible);

	Text("Demos:");
	BeginChild("demo_list", ImVec2(0, 0), true);
	Text("...");
	EndChild();

	End();
}

} // namespace
