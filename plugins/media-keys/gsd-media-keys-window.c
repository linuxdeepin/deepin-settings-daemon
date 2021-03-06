/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gsd-media-keys-window.h"

#define GSD_MEDIA_KEYS_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MEDIA_KEYS_WINDOW, GsdMediaKeysWindowPrivate))

/* The size of the icon compared to the whole OSD */
#define ICON_SCALE 0.50

struct GsdMediaKeysWindowPrivate
{
        GsdMediaKeysWindowAction action;
        char                    *icon_name;
        gboolean                 show_level;

        guint                    volume_muted : 1;
        int                      volume_level;
};

G_DEFINE_TYPE (GsdMediaKeysWindow, gsd_media_keys_window, GSD_TYPE_OSD_WINDOW)

static const char *
get_image_name_for_volume (gboolean muted,
			   int volume)
{
        static const char *icon_names[] = {
                "audio-volume-muted-symbolic",
                "audio-volume-low-symbolic",
                "audio-volume-medium-symbolic",
                "audio-volume-high-symbolic",
                NULL
        };
        int n;

        if (muted) {
                n = 0;
        } else {
                /* select image */
                n = 3 * volume / 100 + 1;
                if (n < 1) {
                        n = 1;
                } else if (n > 3) {
                        n = 3;
                }
        }

	return icon_names[n];
}

static void
action_changed (GsdMediaKeysWindow *window)
{
        gsd_osd_window_update_and_hide (GSD_OSD_WINDOW (window));
}

static void
volume_level_changed (GsdMediaKeysWindow *window)
{
        gsd_osd_window_update_and_hide (GSD_OSD_WINDOW (window));
}

static void
volume_muted_changed (GsdMediaKeysWindow *window)
{
        gsd_osd_window_update_and_hide (GSD_OSD_WINDOW (window));
}

void
gsd_media_keys_window_set_action (GsdMediaKeysWindow      *window,
                                  GsdMediaKeysWindowAction action)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));
        g_return_if_fail (action == GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME);

        if (window->priv->action != action) {
                window->priv->action = action;
                action_changed (window);
        } else {
                gsd_osd_window_update_and_hide (GSD_OSD_WINDOW (window));
        }
}

void
gsd_media_keys_window_set_action_custom (GsdMediaKeysWindow      *window,
                                         const char              *icon_name,
                                         gboolean                 show_level)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));
        g_return_if_fail (icon_name != NULL);

        if (window->priv->action != GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM ||
            g_strcmp0 (window->priv->icon_name, icon_name) != 0 ||
            window->priv->show_level != show_level) {
                window->priv->action = GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM;
                g_free (window->priv->icon_name);
                window->priv->icon_name = g_strdup (icon_name);
                window->priv->show_level = show_level;
                action_changed (window);
        } else {
                gsd_osd_window_update_and_hide (GSD_OSD_WINDOW (window));
        }
}

void
gsd_media_keys_window_set_volume_muted (GsdMediaKeysWindow *window,
                                        gboolean            muted)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));

        if (window->priv->volume_muted != muted) {
                window->priv->volume_muted = muted;
                volume_muted_changed (window);
        }
}

void
gsd_media_keys_window_set_volume_level (GsdMediaKeysWindow *window,
                                        int                 level)
{
        g_return_if_fail (GSD_IS_MEDIA_KEYS_WINDOW (window));

        if (window->priv->volume_level != level) {
                window->priv->volume_level = level;
                volume_level_changed (window);
        }
}

static GdkPixbuf *
load_pixbuf (GsdMediaKeysWindow *window,
             const char         *name,
             int                 icon_size)
{
        GtkIconTheme    *theme;
        GtkIconInfo     *info;
        GdkPixbuf       *pixbuf;
        GtkStyleContext *context;
        GdkRGBA          color;

        if (window != NULL && gtk_widget_has_screen (GTK_WIDGET (window))) {
                theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (window)));
        } else {
                theme = gtk_icon_theme_get_default ();
        }

        context = gtk_widget_get_style_context (GTK_WIDGET (window));
        gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &color);
        info = gtk_icon_theme_lookup_icon (theme,
                                           name,
                                           icon_size,
                                           GTK_ICON_LOOKUP_FORCE_SIZE | GTK_ICON_LOOKUP_GENERIC_FALLBACK);

        if (info == NULL) {
                g_warning ("Failed to load '%s'", name);
                return NULL;
        }

        pixbuf = gtk_icon_info_load_symbolic (info,
                                              &color,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL);
        /*gtk_icon_info_free (info);*/
        g_object_unref (info);

        return pixbuf;
}

static void
draw_eject (cairo_t *cr,
            double   _x0,
            double   _y0,
            double   width,
            double   height)
{
        int box_height;
        int tri_height;
        int separation;

        box_height = height * 0.2;
        separation = box_height / 3;
        tri_height = height - box_height - separation;

        cairo_rectangle (cr, _x0, _y0 + height - box_height, width, box_height);

        cairo_move_to (cr, _x0, _y0 + tri_height);
        cairo_rel_line_to (cr, width, 0);
        cairo_rel_line_to (cr, -width / 2, -tri_height);
        cairo_rel_line_to (cr, -width / 2, tri_height);
        cairo_close_path (cr);
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, GSD_OSD_WINDOW_FG_ALPHA);
        cairo_fill_preserve (cr);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, GSD_OSD_WINDOW_FG_ALPHA / 2);
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
}

static void
draw_waves (cairo_t *cr,
            double   cx,
            double   cy,
            double   max_radius,
            int      volume_level)
{
        const int n_waves = 3;
        int last_wave;
        int i;

        last_wave = n_waves * volume_level / 100;

        for (i = 0; i < n_waves; i++) {
                double angle1;
                double angle2;
                double radius;
                double alpha;

                angle1 = -M_PI / 4;
                angle2 = M_PI / 4;

                if (i < last_wave)
                        alpha = 1.0;
                else if (i > last_wave)
                        alpha = 0.1;
                else alpha = 0.1 + 0.9 * (n_waves * volume_level % 100) / 100.0;

                radius = (i + 1) * (max_radius / n_waves);
                cairo_arc (cr, cx, cy, radius, angle1, angle2);
                cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, alpha / 2);
                cairo_set_line_width (cr, 14);
                cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke_preserve (cr);

                cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, alpha);
                cairo_set_line_width (cr, 10);
                cairo_set_line_cap  (cr, CAIRO_LINE_CAP_ROUND);
                cairo_stroke (cr);
        }
}

static void
draw_cross (cairo_t *cr,
            double   cx,
            double   cy,
            double   size)
{
        cairo_move_to (cr, cx, cy - size/2.0);
        cairo_rel_line_to (cr, size, size);

        cairo_move_to (cr, cx, cy + size/2.0);
        cairo_rel_line_to (cr, size, -size);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, GSD_OSD_WINDOW_FG_ALPHA / 2);
        cairo_set_line_width (cr, 14);
        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke_preserve (cr);

        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, GSD_OSD_WINDOW_FG_ALPHA);
        cairo_set_line_width (cr, 10);
        cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
        cairo_stroke (cr);
}

static void
draw_speaker (cairo_t *cr,
              double   cx,
              double   cy,
              double   width,
              double   height)
{
        double box_width;
        double box_height;
        double _x0;
        double _y0;

        box_width = width / 3;
        box_height = height / 3;

        _x0 = cx - (width / 2) + box_width;
        _y0 = cy - box_height / 2;

        cairo_move_to (cr, _x0, _y0);
        cairo_rel_line_to (cr, - box_width, 0);
        cairo_rel_line_to (cr, 0, box_height);
        cairo_rel_line_to (cr, box_width, 0);

        cairo_line_to (cr, cx + box_width, cy + height / 2);
        cairo_rel_line_to (cr, 0, -height);
        cairo_line_to (cr, _x0, _y0);
        cairo_close_path (cr);

        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, GSD_OSD_WINDOW_FG_ALPHA);
        cairo_fill_preserve (cr);

        cairo_set_source_rgba (cr, 0.6, 0.6, 0.6, GSD_OSD_WINDOW_FG_ALPHA / 2);
        cairo_set_line_width (cr, 2);
        cairo_stroke (cr);
}

static gboolean
render_speaker (GsdMediaKeysWindow *window,
                cairo_t            *cr,
                double              _x0,
                double              _y0,
                double              width,
                double              height)
{
        GdkPixbuf         *pixbuf;
        const char        *icon_name;
        int                icon_size;

	icon_name = get_image_name_for_volume (window->priv->volume_muted,
					       window->priv->volume_level);

        icon_size = (int)width;

        pixbuf = load_pixbuf (window, icon_name, icon_size);

        if (pixbuf == NULL) {
                return FALSE;
        }

        gdk_cairo_set_source_pixbuf (cr, pixbuf, _x0, _y0);
        cairo_paint_with_alpha (cr, GSD_OSD_WINDOW_FG_ALPHA);

        g_object_unref (pixbuf);

        return TRUE;
}

#define LIGHTNESS_MULT  1.3
#define DARKNESS_MULT   0.7

static void
draw_volume_boxes (GsdMediaKeysWindow *window,
                   cairo_t            *cr,
                   double              percentage,
                   double              _x0,
                   double              _y0,
                   double              width,
                   double              height)
{
        gdouble   x1;
        GtkStyleContext *context;
        GdkRGBA  acolor;

        height = round (height) - 1;
        width = round (width) - 1;
        x1 = round ((width - 1) * percentage);
        context = gtk_widget_get_style_context (GTK_WIDGET (window));

        /* bar background */
        gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &acolor);
        gsd_osd_window_color_shade (&acolor, DARKNESS_MULT);
        gsd_osd_window_color_reverse (&acolor);
        acolor.alpha = GSD_OSD_WINDOW_FG_ALPHA / 2;
        gsd_osd_window_draw_rounded_rectangle (cr, 1.0, _x0, _y0, height / 6, width, height);
        gdk_cairo_set_source_rgba (cr, &acolor);
        cairo_fill (cr);

        /* bar progress */
        if (percentage < 0.01)
                return;
        gtk_style_context_get_background_color (context, GTK_STATE_NORMAL, &acolor);
        acolor.alpha = GSD_OSD_WINDOW_FG_ALPHA;
        gsd_osd_window_draw_rounded_rectangle (cr, 1.0, _x0, _y0, height / 6, x1, height);
        gdk_cairo_set_source_rgba (cr, &acolor);
        cairo_fill (cr);
}

static void
draw_action_volume (GsdMediaKeysWindow *window,
                    cairo_t            *cr)
{
        int window_width;
        int window_height;
        double icon_box_width;
        double icon_box_height;
        double icon_box_x0;
        double icon_box_y0;
        double volume_box_x0;
        double volume_box_y0;
        double volume_box_width;
        double volume_box_height;
        gboolean res;

        gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

        icon_box_width = round (window_width * ICON_SCALE);
        icon_box_height = round (window_height * ICON_SCALE);
        volume_box_width = icon_box_width;
        volume_box_height = round (window_height * 0.05);

        icon_box_x0 = round ((window_width - icon_box_width) / 2);
        icon_box_y0 = round ((window_height - icon_box_height - volume_box_height) / 2 - volume_box_height);
        volume_box_x0 = round (icon_box_x0);
        volume_box_y0 = round (icon_box_height + icon_box_y0) + volume_box_height;

#if 0
        g_message ("icon box: w=%f h=%f _x0=%f _y0=%f",
                   icon_box_width,
                   icon_box_height,
                   icon_box_x0,
                   icon_box_y0);
        g_message ("volume box: w=%f h=%f _x0=%f _y0=%f",
                   volume_box_width,
                   volume_box_height,
                   volume_box_x0,
                   volume_box_y0);
#endif

        res = render_speaker (window,
                              cr,
                              icon_box_x0, icon_box_y0,
                              icon_box_width, icon_box_height);
        if (! res) {
                double speaker_width;
                double speaker_height;
                double speaker_cx;
                double speaker_cy;

                speaker_width = icon_box_width * 0.5;
                speaker_height = icon_box_height * 0.75;
                speaker_cx = icon_box_x0 + speaker_width / 2;
                speaker_cy = icon_box_y0 + speaker_height / 2;

#if 0
                g_message ("speaker box: w=%f h=%f cx=%f cy=%f",
                           speaker_width,
                           speaker_height,
                           speaker_cx,
                           speaker_cy);
#endif

                /* draw speaker symbol */
                draw_speaker (cr, speaker_cx, speaker_cy, speaker_width, speaker_height);

                if (! window->priv->volume_muted) {
                        /* draw sound waves */
                        double wave_x0;
                        double wave_y0;
                        double wave_radius;

                        wave_x0 = window_width / 2;
                        wave_y0 = speaker_cy;
                        wave_radius = icon_box_width / 2;

                        draw_waves (cr, wave_x0, wave_y0, wave_radius, window->priv->volume_level);
                } else {
                        /* draw 'mute' cross */
                        double cross_x0;
                        double cross_y0;
                        double cross_size;

                        cross_size = speaker_width * 3 / 4;
                        cross_x0 = icon_box_x0 + icon_box_width - cross_size;
                        cross_y0 = speaker_cy;

                        draw_cross (cr, cross_x0, cross_y0, cross_size);
                }
        }

        /* draw volume meter */
        draw_volume_boxes (window,
                           cr,
                           (double)window->priv->volume_level / 100.0,
                           volume_box_x0,
                           volume_box_y0,
                           volume_box_width,
                           volume_box_height);
}

static gboolean
render_custom (GsdMediaKeysWindow *window,
               cairo_t            *cr,
               double              _x0,
               double              _y0,
               double              width,
               double              height)
{
        GdkPixbuf         *pixbuf;
        int                icon_size;

        icon_size = (int)width;

        pixbuf = load_pixbuf (window, window->priv->icon_name, icon_size);

        if (pixbuf == NULL) {
                char *name;
                if (gtk_widget_get_direction (GTK_WIDGET (window)) == GTK_TEXT_DIR_RTL)
                        name = g_strdup_printf ("%s-rtl", window->priv->icon_name);
                else
                        name = g_strdup_printf ("%s-ltr", window->priv->icon_name);
                pixbuf = load_pixbuf (window, name, icon_size);
                g_free (name);
                if (pixbuf == NULL)
                        return FALSE;
        }

        gdk_cairo_set_source_pixbuf (cr, pixbuf, _x0, _y0);
        cairo_paint_with_alpha (cr, GSD_OSD_WINDOW_FG_ALPHA);

        g_object_unref (pixbuf);

        return TRUE;
}

static void
draw_action_custom (GsdMediaKeysWindow *window,
                    cairo_t            *cr)
{
        int window_width;
        int window_height;
        double icon_box_width;
        double icon_box_height;
        double icon_box_x0;
        double icon_box_y0;
        double bright_box_x0;
        double bright_box_y0;
        double bright_box_width;
        double bright_box_height;
        gboolean res;

        gtk_window_get_size (GTK_WINDOW (window), &window_width, &window_height);

        icon_box_width = round (window_width * ICON_SCALE);
        icon_box_height = round (window_height * ICON_SCALE);
        bright_box_width = round (icon_box_width);
        bright_box_height = round (window_height * 0.05);

        icon_box_x0 = round ((window_width - icon_box_width) / 2);
        icon_box_y0 = round ((window_height - icon_box_height - bright_box_height) / 2 - bright_box_height);
        bright_box_x0 = round (icon_box_x0);
        bright_box_y0 = round (icon_box_height + icon_box_y0) + bright_box_height;

#if 0
        g_message ("icon box: w=%f h=%f _x0=%f _y0=%f",
                   icon_box_width,
                   icon_box_height,
                   icon_box_x0,
                   icon_box_y0);
        g_message ("brightness box: w=%f h=%f _x0=%f _y0=%f",
                   bright_box_width,
                   bright_box_height,
                   bright_box_x0,
                   bright_box_y0);
#endif

        res = render_custom (window,
                             cr,
                             icon_box_x0, icon_box_y0,
                             icon_box_width, icon_box_height);
        if (! res && g_str_has_prefix (window->priv->icon_name, "media-eject")) {
                /* draw eject symbol */
                draw_eject (cr,
                            icon_box_x0, icon_box_y0,
                            icon_box_width, icon_box_height);
        }

        if (window->priv->show_level != FALSE) {
                /* draw volume meter */
                draw_volume_boxes (window,
                                   cr,
                                   (double)window->priv->volume_level / 100.0,
                                   bright_box_x0,
                                   bright_box_y0,
                                   bright_box_width,
                                   bright_box_height);
        }
}

static void
gsd_media_keys_window_draw_when_composited (GsdOsdWindow *osd_window,
                                            cairo_t      *cr)
{
        GsdMediaKeysWindow *window = GSD_MEDIA_KEYS_WINDOW (osd_window);

        switch (window->priv->action) {
        case GSD_MEDIA_KEYS_WINDOW_ACTION_VOLUME:
                draw_action_volume (window, cr);
                break;
        case GSD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM:
                draw_action_custom (window, cr);
                break;
        default:
                break;
        }
}

static void
gsd_media_keys_window_class_init (GsdMediaKeysWindowClass *klass)
{
        GsdOsdWindowClass *osd_window_class = GSD_OSD_WINDOW_CLASS (klass);

        osd_window_class->draw_when_composited = gsd_media_keys_window_draw_when_composited;

        g_type_class_add_private (klass, sizeof (GsdMediaKeysWindowPrivate));
}

static void
gsd_media_keys_window_init (GsdMediaKeysWindow *window)
{
        window->priv = GSD_MEDIA_KEYS_WINDOW_GET_PRIVATE (window);
}

GtkWidget *
gsd_media_keys_window_new (void)
{
        return g_object_new (GSD_TYPE_MEDIA_KEYS_WINDOW, NULL);
}
