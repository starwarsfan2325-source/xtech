#ifndef XPP_XGUI_FILE_SELECT_HPP
#define XPP_XGUI_FILE_SELECT_HPP

#include "window.hpp"
#include <filesystem>

namespace xpp::xgui {
	struct dir_t {
		std::filesystem::path path;
		std::vector<dir_t> children;
		bool touched = false;
	};

	struct file_select_w : window_t {
		ImGuiTextFilter filter;
		std::vector<std::filesystem::directory_entry> files;
		std::vector<dir_t> folders;
		std::string extension = "";
		std::filesystem::path directory;
		std::filesystem::path selected_dir;
		std::filesystem::path selected_file;
		std::filesystem::path opened_file;
		bool is_open = false;

		file_select_w(std::string title = "Open File");
		~file_select_w(void);
		void open(std::string dir = ".", std::string ext = "", bool reset = false);
		void draw(void);
		bool file_is_valid(std::filesystem::path file);
		void recurse(dir_t &dir);
		void refresh_dir(dir_t &dir);
		void refresh_files(void);
		void cd(std::filesystem::path dir);
	};
}

#endif
