#ifndef XPP_XGUI_MEMORY_HPP
#define XPP_XGUI_MEMORY_HPP

#include "window.hpp"
#include <vector>
#include <string>
#include <stdint.h>

namespace xpp::xgui {
	enum WATCH_DATA_TYPE {
		WATCH_U8,
		WATCH_U16,
		WATCH_16,
	};

	enum SEARCH_CMP {
		CMP_EQ,
		CMP_NOT,
		CMP_LT,
		CMP_GT,
		CMP_LE,
		CMP_GE,
	};

	struct memory_watch_t {
		size_t address;
		unsigned int value;
		unsigned int last;
		WATCH_DATA_TYPE type;
		std::string notes;
	};

	struct memory_search_t {
		size_t address;
		unsigned int value;
		unsigned int last;
	};

	struct memory_w : window_t {
		std::vector<memory_watch_t> memory_watches;
		uint8_t enemy_id = 0;

		memory_w(void);
		~memory_w(void);
		bool memory_watch_has_address(size_t address);
		bool memory_watch_push(size_t address, std::string notes);
		void memory_search_push(size_t address);
		void save_watches(std::string filename);
		void load_watches(std::string filename);
		void update_watches(void);
		void update_search(void);
		void filter_addresses(void);
		void filter_addresses_lua(void);
		void reset_search(void);
		void draw_watch(void);
		void draw_search(void);
		void draw_objects(void);
		void draw(void);
	};
}

#endif
