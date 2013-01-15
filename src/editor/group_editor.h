#ifndef EDITOR_GROUP_EDITOR_H
#define EDITOR_GROUP_EDITOR_H

#include "base.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

namespace ui {

	class GroupEditor: public ui::Window {
	public:
		GroupEditor(IRect rect);

		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);
		virtual void drawContents() const;

		void setTarget(TileGroup* tile_group);
		void setTileFilter(TileFilter::Type);
		TileFilter::Type tileFilter() const { return m_tile_filter; }

	protected:
		void updateSelector();

		ui::TileList m_tile_list;
		TileGroup *m_tile_group;

		gfx::PFont m_font;
		IRect m_view;

		TileFilter::Type m_tile_filter;

		int2 m_offset[3];
		const ui::TileList::Entry *m_current_entry;
		int m_selected_group_id;
		int m_selected_surface_id;
		int m_select_mode;
		int m_selection_mode;
		enum {
			mAddRemove, // add / remove tiles to / from group
			mModify, // modify tiles relations
		} m_mode;
	};

	typedef Ptr<GroupEditor> PGroupEditor;

}


#endif
