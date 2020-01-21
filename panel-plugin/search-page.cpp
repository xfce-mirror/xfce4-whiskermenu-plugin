/*
 * Copyright (C) 2013-2020 Graeme Gott <graeme@gottcode.org>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "search-page.h"

#include "launcher.h"
#include "launcher-view.h"
#include "search-action.h"
#include "settings.h"
#include "slot.h"
#include "window.h"

#include <algorithm>

#include <gdk/gdkkeysyms.h>

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

SearchPage::SearchPage(Window* window) :
	Page(window)
{
	view_created();

	g_signal_connect_slot(window->get_search_entry(), "activate", &SearchPage::activate_search, this);
	g_signal_connect_slot(window->get_search_entry(), "stop-search", &SearchPage::cancel_search, this);
}

//-----------------------------------------------------------------------------

SearchPage::~SearchPage()
{
	unset_menu_items();
}

//-----------------------------------------------------------------------------

void SearchPage::set_filter(const gchar* filter)
{
	// Clear search results for empty filter
	if (!filter)
	{
		m_query.clear();
		m_matches.clear();
		return;
	}

	// Make sure this is a new search
	std::string query(filter);
	if (m_query.raw_query() == query)
	{
		return;
	}

	// Reset search results if new search does not start with previous search
	if (m_query.raw_query().empty() || !g_str_has_prefix(filter, m_query.raw_query().c_str()))
	{
		m_matches.clear();
		m_matches.push_back(&m_run_action);
		for (auto launcher : m_launchers)
		{
			m_matches.push_back(launcher);
		}
	}
	else if (std::find(m_matches.begin(), m_matches.end(), &m_run_action) == m_matches.end())
	{
		m_matches.insert(m_matches.begin(), &m_run_action);
	}
	m_query.set(query);

	// Create search results
	std::vector<Match> search_action_matches;
	search_action_matches.reserve(wm_settings->search_actions.size());
	for (auto action : wm_settings->search_actions)
	{
		Match match(action);
		match.update(m_query);
		if (!Match::invalid(match))
		{
			search_action_matches.push_back(match);
		}
	}
	std::stable_sort(search_action_matches.begin(), search_action_matches.end());
	std::reverse(search_action_matches.begin(), search_action_matches.end());

	for (auto& match : m_matches)
	{
		match.update(m_query);
	}
	m_matches.erase(std::remove_if(m_matches.begin(), m_matches.end(), &Match::invalid), m_matches.end());
	std::stable_sort(m_matches.begin(), m_matches.end());

	// Show search results
	GtkListStore* store = gtk_list_store_new(
			LauncherView::N_COLUMNS,
			G_TYPE_ICON,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_POINTER);
	Element* element = nullptr;
	for (const auto& match : search_action_matches)
	{
		element = match.element();
		gtk_list_store_insert_with_values(
				store, nullptr, G_MAXINT,
				LauncherView::COLUMN_ICON, element->get_icon(),
				LauncherView::COLUMN_TEXT, element->get_text(),
				LauncherView::COLUMN_TOOLTIP, element->get_tooltip(),
				LauncherView::COLUMN_LAUNCHER, element,
				-1);
	}
	for (const auto& match : m_matches)
	{
		element = match.element();
		gtk_list_store_insert_with_values(
				store, nullptr, G_MAXINT,
				LauncherView::COLUMN_ICON, element->get_icon(),
				LauncherView::COLUMN_TEXT, element->get_text(),
				LauncherView::COLUMN_TOOLTIP, element->get_tooltip(),
				LauncherView::COLUMN_LAUNCHER, element,
				-1);
	}
	get_view()->set_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	// Find first result
	GtkTreeIter iter;
	GtkTreePath* path = gtk_tree_path_new_first();
	bool found = gtk_tree_model_get_iter(get_view()->get_model(), &iter, path);

	// Scroll to and select first result
	if (found)
	{
		get_view()->select_path(path);
		get_view()->scroll_to_path(path);
	}
	gtk_tree_path_free(path);
}

//-----------------------------------------------------------------------------

void SearchPage::set_menu_items(GtkTreeModel* model)
{
	// loop over every single item in model
	GtkTreeIter iter;
	bool valid = gtk_tree_model_get_iter_first(model, &iter);
	while (valid)
	{
		Launcher* launcher = nullptr;
		gtk_tree_model_get(model, &iter, LauncherView::COLUMN_LAUNCHER, &launcher, -1);
		if (launcher)
		{
			m_launchers.push_back(launcher);
		}
		valid = gtk_tree_model_iter_next(model, &iter);
	}

	get_view()->unset_model();

	m_matches.clear();
	m_matches.reserve(m_launchers.size() + 1);
}

//-----------------------------------------------------------------------------

void SearchPage::unset_menu_items()
{
	m_launchers.clear();
	m_matches.clear();
	get_view()->unset_model();
}

//-----------------------------------------------------------------------------

void SearchPage::activate_search(GtkEntry* entry)
{
	set_filter(gtk_entry_get_text(entry));
	GtkTreePath* path = get_view()->get_selected_path();
	if (path)
	{
		get_view()->activate_path(path);
		gtk_tree_path_free(path);
	}
}

//-----------------------------------------------------------------------------

void SearchPage::cancel_search(GtkSearchEntry* entry)
{
	const gchar* text = gtk_entry_get_text(GTK_ENTRY(entry));
	if (!exo_str_is_empty(text))
	{
		gtk_entry_set_text(GTK_ENTRY(entry), "");
	}
}

//-----------------------------------------------------------------------------

void SearchPage::view_created()
{
	get_view()->set_selection_mode(GTK_SELECTION_BROWSE);
}

//-----------------------------------------------------------------------------
