/*
 * Copyright (C) 2014-2020 Graeme Gott <graeme@gottcode.org>
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

#ifndef WHISKERMENU_PROFILE_PICTURE_H
#define WHISKERMENU_PROFILE_PICTURE_H

#include <gtk/gtk.h>

#ifdef HAS_ACCOUNTSERVICE
#include <act/act.h>
#endif

namespace WhiskerMenu
{

class Window;

class ProfilePicture
{
public:
	explicit ProfilePicture(Window* window);
	~ProfilePicture();

	ProfilePicture(const ProfilePicture&) = delete;
	ProfilePicture(ProfilePicture&&) = delete;
	ProfilePicture& operator=(const ProfilePicture&) = delete;
	ProfilePicture& operator=(ProfilePicture&&) = delete;

	GtkWidget* get_widget() const
	{
		return m_container;
	}

	void reset_tooltip();

private:
	void update_profile_picture();
#ifdef HAS_ACCOUNTSERVICE
	void on_user_changed(ActUserManager* um, ActUser* user);
	void on_user_loaded(ActUser* user, GParamSpec* param);
	void on_user_info_loaded(ActUserManager* um, GParamSpec* param);
#else
	void on_file_changed(GFileMonitor* monitor, GFile* file, GFile* other_file, GFileMonitorEvent event_type);
#endif
	void on_button_press_event();

private:
	Window* m_window;
	GtkWidget* m_container;
	GtkWidget* m_image;
#ifdef HAS_ACCOUNTSERVICE
	ActUserManager* m_act_user_manager;
	ActUser* m_act_user;
#else
	GFileMonitor* m_file_monitor;
#endif
	gchar *m_file_path;
};

}

#endif // WHISKERMENU_PROFILE_PICTURE_H
