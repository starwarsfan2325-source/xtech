#ifndef XPP_XGUI_DOCS_HPP
#define XPP_XGUI_DOCS_HPP

#include "window.hpp"
#include <filesystem>

namespace xpp::xgui {
	struct doc_t {
		std::filesystem::path path;
		bool touched = false;
		std::string body = "";
	};

	struct docs_w : window_t {
		std::filesystem::path selected_doc;
		std::vector<doc_t> docs;
		std::filesystem::path directory;
		std::string *body_ptr;
		char *editbuf;
		bgfx::TextureHandle _t;

		docs_w(void);
		~docs_w(void);
		void draw(void);
		void refresh_docs(std::filesystem::path dir);
		void load_doc(doc_t &doc);
	};
}

#endif
