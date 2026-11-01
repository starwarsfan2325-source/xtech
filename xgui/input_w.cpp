#include "input_w.hpp"

using namespace ImGui;

namespace xpp::xgui {

input_w::input_w(void) : window_t("Input")
{
}

input_w::~input_w(void)
{
}

void
input_w::draw(void)
{
	Begin(ICON_FA_GAMEPAD " Input", &visible);

	Text("Bindings:");

	End();
}

} // namespace
