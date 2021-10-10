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

#include "profile-picture.h"

#include "command.h"
#include "settings.h"
#include "slot.h"
#include "window.h"

#include <libxfce4panel/libxfce4panel.h>

using namespace WhiskerMenu;

//-----------------------------------------------------------------------------

ProfilePicture::ProfilePicture(Window* window) :
	m_window(window)
{
	m_image = gtk_image_new();

	gtk_widget_set_halign(m_image, GTK_ALIGN_CENTER);
	gtk_widget_set_valign(m_image, GTK_ALIGN_CENTER);

	m_container = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(m_container), false);
	gtk_widget_add_events(m_container, GDK_BUTTON_PRESS_MASK);
	g_signal_connect_slot<GtkWidget*, GdkEvent*>(m_container, "button-press-event", &ProfilePicture::on_button_press_event, this);
	gtk_container_add(GTK_CONTAINER(m_container), m_image);

	Command* command = wm_settings->command[Settings::CommandProfile];
	gtk_widget_set_tooltip_text(m_container, command->get_tooltip());

#if HAVE_ACCOUNTSERVICE
	m_act_um = act_user_manager_get_default();
	gboolean loaded = FALSE;
	g_object_get (m_act_um, "is-loaded", &loaded, nullptr);
	if (loaded)
	{
		on_user_info_loaded (m_act_um, nullptr);
	}
	else
	{
		g_signal_connect_slot(m_act_um, "notify::is-loaded", &ProfilePicture::on_user_info_loaded, this);
	}
#else
	gchar* path = g_build_filename(g_get_home_dir(), ".face", nullptr);
	GFile* file = g_file_new_for_path(path);
	g_free(path);

	m_file_monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
	g_signal_connect_slot(m_file_monitor, "changed", &ProfilePicture::on_file_changed, this);
	on_file_changed(m_file_monitor, file, nullptr, G_FILE_MONITOR_EVENT_CHANGED);

	g_object_unref(file);
#endif
}

//-----------------------------------------------------------------------------

ProfilePicture::~ProfilePicture()
{
#if HAVE_ACCOUNTSERVICE
	g_object_unref (m_act_um);
	g_object_unref (m_act_user);
#else
	g_file_monitor_cancel(m_file_monitor);
	g_object_unref(m_file_monitor);
#endif
}

//-----------------------------------------------------------------------------

void ProfilePicture::reset_tooltip()
{
	Command* command = wm_settings->command[Settings::CommandProfile];
	gtk_widget_set_has_tooltip(m_container, command->get_shown());
}

//-----------------------------------------------------------------------------

static GdkPixbuf *round_pixbuf_file_at_size (const gchar* file, gint size)
{
	GdkPixbuf *pixbuf = nullptr;
	GdkPixbuf *dest = nullptr;
	cairo_surface_t *surface;
	cairo_t *cr;

	pixbuf = gdk_pixbuf_new_from_file_at_size(file, size, size, nullptr);
	if (!pixbuf)
		return nullptr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, size, size);
	cr = cairo_create (surface);

	cairo_arc (cr, size/2, size/2, size/2, 0, 2 * G_PI);
	cairo_clip (cr);
	cairo_new_path (cr);

	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_paint (cr);

	dest = gdk_pixbuf_get_from_surface (surface, 0, 0, size, size);
	cairo_surface_destroy (surface);
	cairo_destroy (cr);

	g_object_unref(pixbuf);

	return dest;
}

//-----------------------------------------------------------------------------

static void set_file_picture(GtkWidget *image, const gchar* file)
{
	GdkPixbuf* pixbuf = nullptr;
	if (file && g_file_test (file, G_FILE_TEST_EXISTS))
	{
		pixbuf = round_pixbuf_file_at_size (file, 32);
	}

	if (pixbuf)
	{
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
		g_object_unref(pixbuf);
	}
	else
	{
		gtk_image_set_from_icon_name(GTK_IMAGE(image), "avatar-default", GTK_ICON_SIZE_DND);
	}
}

#if HAVE_ACCOUNTSERVICE
//-----------------------------------------------------------------------------

void ProfilePicture::on_user_changed(ActUserManager*, ActUser* user)
{
	if (act_user_get_uid (user) != getuid ())
		return;

	set_file_picture (m_image, act_user_get_icon_file(user));
}

//-----------------------------------------------------------------------------

void ProfilePicture::on_user_loaded(ActUser* user, GParamSpec*)
{
	on_user_changed(nullptr, user);
}

//-----------------------------------------------------------------------------

void ProfilePicture::on_user_info_loaded(ActUserManager*, GParamSpec*)
{
	if (act_user_manager_no_service(m_act_um))
	{
		gtk_image_set_from_icon_name(GTK_IMAGE(m_image), "avatar-default", GTK_ICON_SIZE_DND);
		return;
	}

	g_signal_connect_slot(m_act_um, "user-changed", &ProfilePicture::on_user_changed, this);

	m_act_user = act_user_manager_get_user_by_id(m_act_um, getuid());
	if (act_user_is_loaded (m_act_user)) {
		on_user_changed(nullptr, m_act_user);
	}
	else {
		g_signal_connect_slot(m_act_user, "notify::is-loaded", &ProfilePicture::on_user_loaded, this);
	}
}
#else
void ProfilePicture::on_file_changed(GFileMonitor*, GFile* file, GFile*, GFileMonitorEvent)
{
	gchar *path = g_file_get_path(file);
	set_file_picture (m_image, path);
	g_free (path);
}
#endif

//-----------------------------------------------------------------------------

void ProfilePicture::on_button_press_event()
{
	Command* command = wm_settings->command[Settings::CommandProfile];
	if (!command->get_shown())
	{
		return;
	}

	m_window->hide();
	command->activate();
}

//-----------------------------------------------------------------------------
