/* The default UI font: Montserrat Bold, compiled in so every platform looks the same. */
#ifndef RECOMP_GUI_FONT_MONTSERRAT_H
#define RECOMP_GUI_FONT_MONTSERRAT_H

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char gui_font_montserrat_bold[];
extern const unsigned int  gui_font_montserrat_bold_size;
/* Its GPOS pair kerning, ready for gui_font_set_kerning(). */
extern const short gui_font_montserrat_bold_kern[][3];
extern const int   gui_font_montserrat_bold_kern_count;

#ifdef __cplusplus
}
#endif
#endif
