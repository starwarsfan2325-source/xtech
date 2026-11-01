#include "about_w.hpp"

using namespace ImGui;

namespace xpp::xgui {

about_w::about_w(void) : window_t("About")
{
	under_menu = "Help";
	switch (bgfx::getRendererType()) {
	case bgfx::RendererType::Vulkan:
		renderer = "Vulkan";
		break;
	case bgfx::RendererType::Metal:
		renderer = "Metal";
		break;
	case bgfx::RendererType::OpenGL:
	case bgfx::RendererType::OpenGLES:
		renderer = "OpenGL";
		break;
	case bgfx::RendererType::Direct3D9:
	case bgfx::RendererType::Direct3D11:
	case bgfx::RendererType::Direct3D12:
		renderer = "Direct3D";
		break;
	}

	if (lua["_VERSION"].valid())
		lua_version = lua["_VERSION"];
}

about_w::~about_w(void)
{
}

void
about_w::draw(void)
{
	Begin(ICON_FA_INFO_CIRCLE " About", &visible);

	Text("Mega Man X++  " XPP_VERSION);
	Separator();

	BeginGroup();
	Text("Commit: ");
	Text("Lua version: ");
	Text("Renderer: ");
	Text("FPS: ");
	EndGroup();

	SameLine();

	BeginGroup();
	Text("%s", "de3e22b");
	Text("%s", lua_version.c_str());
	Text("%s", renderer.c_str());
	Text("%.1f", 60.0f);
	//Text("%.1f", GetIO().Framerate); // TODO why is this broken?
	EndGroup();

	//Separator();

	Text("Render info:");
	SameLine();
	SmallButton("Copy");
	//BeginChild("render_info", ImVec2(0, 297), true);
	BeginChild("render_info", ImVec2(0, 0), true);
	const bgfx::Caps *caps = bgfx::getCaps();
	PushFont(font_mono);

	Text("Renderer                    %s", renderer.c_str());
	Text("Number of GPUs              %d", caps->numGPUs + 1);
	Text("Instancing supported        %s", (caps->supported | BGFX_CAPS_INSTANCING) ? "Yes" : "No");
	Text("Max draw calls              %d", caps->limits.maxDrawCalls);
	Text("Max blits                   %d", caps->limits.maxBlits);
	Text("Max textures                %d", caps->limits.maxTextures);
	Text("Max texture size            %d", caps->limits.maxTextureSize);
	Text("Max views                   %d", caps->limits.maxViews);
	Text("Max frame buffers           %d", caps->limits.maxFrameBuffers);
	Text("Max index buffers           %d", caps->limits.maxIndexBuffers);
	Text("Max vertex buffers          %d", caps->limits.maxVertexBuffers);
	Text("Max dynamic index buffers   %d", caps->limits.maxDynamicIndexBuffers);
	Text("Max dynamic vertex buffers  %d", caps->limits.maxDynamicVertexBuffers);
	Text("Max transient vertices      %d", caps->limits.transientVbSize);
	Text("Max transient incidices     %d", caps->limits.transientIbSize);

	PopFont();
	EndChild();

	//Separator();

	/*Text("Loaded scripts:");
	BeginChild("loaded_scripts", ImVec2(0, 0), true);
	Text("...");
	EndChild();*/

	End();
}

} // namespace
