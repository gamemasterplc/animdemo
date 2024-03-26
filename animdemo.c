#include "libdragon.h"
#include "animsprite.h"
#include "t3ddebug.h"

#include <malloc.h>
#include <math.h>

static sprite_t *tiles_sprite;
static AnimSprite *anim_sprite;

void render(int cur_frame)
{
    // Attach and clear the screen
    surface_t *disp = display_get();
    rdpq_attach_clear(disp, NULL);

    rdpq_set_mode_copy(false);
    uint32_t display_width = display_get_width();
    uint32_t display_height = display_get_height();
	
	surface_t tiles_surf = sprite_get_pixels(tiles_sprite);

    // Check if the sprite was compiled with a paletted format. Normally
    // we should know this beforehand, but for this demo we pretend we don't
    // know. This also shows how rdpq can transparently work in both modes.
    bool tlut = false;
    tex_format_t tiles_format = sprite_get_format(tiles_sprite);
    if (tiles_format == FMT_CI4 || tiles_format == FMT_CI8) {
        // If the sprite is paletted, turn on palette mode and load the
        // palette in TMEM. We use the mode stack for demonstration,
        // so that we show how a block can temporarily change the current
        // render mode, and then restore it at the end.
        rdpq_mode_push();
        rdpq_mode_tlut(TLUT_RGBA16);
        rdpq_tex_upload_tlut(sprite_get_palette(tiles_sprite), 0, 16);
        tlut = true;
    }
    uint32_t tile_width = tiles_sprite->width / tiles_sprite->hslices;
    uint32_t tile_height = tiles_sprite->height / tiles_sprite->vslices;
	int tileid = 0;
    for (uint32_t ty = 0; ty < display_height; ty += tile_height)
    {
        for (uint32_t tx = 0; tx < display_width; tx += tile_width)
        {
            // Load a random tile among the 4 available in the texture,
            // and draw it as a rectangle.
            // Notice that this code is agnostic to both the texture format
            // and the render mode (standard vs copy), it will work either way.
            int s = (tileid%2)*32, t = ((tileid%4)/2)*32;
            rdpq_tex_upload_sub(TILE0, &tiles_surf, NULL, s, t, s+32, t+32);
            rdpq_texture_rectangle(TILE0, tx, ty, tx+32, ty+32, s, t);
			tileid++;
        }
    }
    
    // Pop the mode stack if we pushed it before
    if (tlut) rdpq_mode_pop();
    
	rdpq_set_mode_standard();
	rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
	rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
	for(int i=0; i<5; i++) {
		rdpq_set_prim_color(RGBA32(255, 0, 0, 20));
		rdpq_fill_rectangle(0, 0, 640, 480);
	}
    // Draw the brew sprites. Use standard mode because copy mode cannot handle
    // scaled sprites.
    rdpq_debug_log_msg("sprites");
    rdpq_set_mode_standard();
    rdpq_mode_filter(FILTER_BILINEAR);
    rdpq_mode_alphacompare(1);                // colorkey (draw pixel with alpha >= 1)
	sprite_t *sprite = AnimSpriteGetSprite(anim_sprite);
	
	rdpq_sprite_blit(sprite, 320, 240, NULL);
	t3d_debug_print_start();
	t3d_debug_printf(530, 36, "%.1f FPS\n", display_get_fps());

    rdpq_detach_show();
}

int main()
{
    debug_init_isviewer();
    debug_init_usblog();

    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);

    joypad_init();
    timer_init();

    dfs_init(DFS_DEFAULT_LOCATION);

    rdpq_init();
    rdpq_debug_start();


    tiles_sprite = sprite_load("rom:/tiles.sprite");

	t3d_debug_print_init();
	
	anim_sprite = AnimSpriteLoad("rom:/paddle.aspr");
    int cur_frame = 0;
    while (1)
    {
        render(cur_frame);
		AnimSpriteUpdate(anim_sprite, 1);
        joypad_poll();
        joypad_buttons_t ckeys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
		
		if(ckeys.c_up) {
			AnimSpriteSetAnim(anim_sprite, "grow");
			AnimSpriteSetLoop(anim_sprite, true);
		}
		if(ckeys.c_down) {
			AnimSpriteSetAnim(anim_sprite, "shrink");
			AnimSpriteSetLoop(anim_sprite, false);
		}
		if(ckeys.c_left) {
			AnimSpriteSetAnim(anim_sprite, "idle_small");
		}
		if(ckeys.c_right) {
			AnimSpriteSetAnim(anim_sprite, "idle_big");
		}
		if(ckeys.start) {
			AnimSpriteSetAnim(anim_sprite, "bounce");
			AnimSpriteSetLoop(anim_sprite, true);
		}
        cur_frame++;
    }
}
