#include "editor/group_pad.h"

namespace ui {

	GroupPad::GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group)
		:Window(rect), m_editor(editor), m_group(group) {
		m_filter_box = new ComboBox(IRect(0, 0, rect.width(), 22), 200,
				"Filter: ", TileFilter::strings(), TileFilter::count);
		attach(m_filter_box.get());
		m_filter_box->selectEntry(editor->tileFilter());
	}

	TileFilter::Type GroupPad::currentFilter() const {
		TileFilter::Type filter = (TileFilter::Type)m_filter_box->selectedId();
		DASSERT(filter >= 0 && filter < TileFilter::count);
		return filter;
	}

	bool GroupPad::onEvent(const Event &ev) {
		if(ev.type == Event::element_selected && m_filter_box.get() == ev.source)
			m_editor->setTileFilter(currentFilter());
		else
			return false;

		return true;
	}



}