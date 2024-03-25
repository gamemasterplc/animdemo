BUILD_DIR=build
include $(N64_INST)/include/n64.mk

ANIMSPR_TOOL = tools/mkanimspr/mkanimspr

src = animdemo.c animsprite.c t3ddebug.c
assets_spranm = $(wildcard assets/*.spranm)
assets_png = tiles.png font.ia4.png

assets_conv = $(addprefix filesystem/,$(notdir $(assets_spranm:%.spranm=%.aspr))) \
	$(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite)))

all: animdemo.z64

filesystem/%.aspr: assets/%.spranm
	@mkdir -p $(dir $@)
	@echo "    [ANIMSPR] $@"
	@$(ANIMSPR_TOOL) --stream $< $@

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/tiles.sprite: MKSPRITE_FLAGS=--format CI4 --tiles 32,32

$(BUILD_DIR)/animdemo.dfs: $(assets_conv)
$(BUILD_DIR)/animdemo.elf: $(src:%.c=$(BUILD_DIR)/%.o)

animdemo.z64: N64_ROM_TITLE="Animation Demo"
animdemo.z64: $(BUILD_DIR)/animdemo.dfs 

clean:
	rm -rf $(BUILD_DIR) $(assets_conv) animdemo.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
