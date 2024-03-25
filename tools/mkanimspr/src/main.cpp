#include "tinyxml2.h"
#include "binwrite.h"
#include "subprocess.h"

#include <vector>
#include <map>

#include <string>
#include <filesystem>

#include <stdarg.h>

namespace fs = std::filesystem;

struct FrameData {
	std::string image;
	uint16_t time;
};

struct AnimData {
	std::string name;
	uint16_t total_time;
	std::vector<FrameData> frames;
};

struct ImageData {
	std::string filename;
	std::string id;
	std::string format;
	std::string dither_algo;
};

struct AnimSprData {
	std::vector<AnimData> anims;
	std::vector<ImageData> images;
	std::map<std::string, uint16_t> image_map;
};

const char *n64_inst = NULL;
bool stream_flag = false;

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    exit(1);
    va_end(args);
}

void ParseAnimations(AnimSprData &animspr, tinyxml2::XMLElement *element)
{
	tinyxml2::XMLElement *anim_element = element->FirstChildElement("animation");
	while(anim_element) {
		AnimData anim;
		uint16_t total_time = 0;
		uint16_t delay_default = anim_element->UnsignedAttribute("delay", 6);
		const char *name = anim_element->Attribute("name");
		if(!name) {
			die("Missing name on animation element\n");
		}
		anim.name = name;
		tinyxml2::XMLElement *frame_element = anim_element->FirstChildElement("frame");
		if(!frame_element) {
			die("Missing frame element\n");
		}
		while(frame_element) {
			FrameData frame;
			const char *image = frame_element->Attribute("image");
			if(!image) {
				die("Missing image on frame element\n");
			}
			frame.image = image;
			frame.time = total_time;
			total_time += frame_element->UnsignedAttribute("delay", delay_default);
			frame_element = frame_element->NextSiblingElement("frame");
			anim.frames.push_back(frame);
		}
		anim.total_time = total_time;
		animspr.anims.push_back(anim);
		anim_element = anim_element->NextSiblingElement("animation");
	}
}

void ParseImages(AnimSprData &animspr, const char *xml_path, tinyxml2::XMLElement *element)
{
	fs::path base_path{xml_path};
	base_path = base_path.parent_path();
	tinyxml2::XMLElement *image_element = element->FirstChildElement("image");
	uint16_t index = 0;
	while(image_element) {
		ImageData image;
		const char *filename = image_element->Attribute("filename");
		if(!filename) {
			die("Missing filename on image element\n");
		}
		const char *id = image_element->Attribute("id");
		if(!id) {
			die("Missing id on image element\n");
		}
		if(animspr.image_map.count(id) != 0) {
			die("Duplicate image ID %s.\n", id);
		}
		const char *format = image_element->Attribute("format");
		const char *dither_algo = image_element->Attribute("dither_algo");
		
		if(!format) {
			format = "AUTO";
		}
		if(!dither_algo) {
			dither_algo = "NONE";
		}
		image.filename = (base_path / filename).string();
		image.id = id;
		image.format = format;
		image.dither_algo = dither_algo;
		animspr.image_map[id] = index;
		animspr.images.push_back(image);
		image_element = image_element->NextSiblingElement("image");
		index++;
	}
}

void ReadXML(const char *path, AnimSprData &animspr)
{
	tinyxml2::XMLDocument document;
	tinyxml2::XMLError error = document.LoadFile(path);
    if(error != tinyxml2::XML_SUCCESS) {
        die("Failed to Load File %s.\n", path);
    }
	tinyxml2::XMLNode *root = document.FirstChild();
    if (!root) {
        die("Failed to Read XML %s.\n", path);
    }
	tinyxml2::XMLElement *animsprite = root->NextSiblingElement("animsprite");
    if (!animsprite) {
        die("XML File has missing animsprite element.\n");
    }
	tinyxml2::XMLElement *animations = animsprite->FirstChildElement("animations");
	tinyxml2::XMLElement *images = animsprite->FirstChildElement("images");
	if(!animations) {
		die("File has no animations.\n");
	}
	if(!images) {
		die("File has no images.\n");
	}
	ParseAnimations(animspr, animations);
	ParseImages(animspr, path, images);
}

void ConvertImage(FILE *file, ImageData *image)
{
    static char *mksprite = NULL;
    if (!mksprite) asprintf(&mksprite, "%s/bin/mksprite", n64_inst);

    // Prepare mksprite command line
    struct subprocess_s subp;
    const char *cmd_addr[16] = {0}; int i = 0;
    cmd_addr[i++] = mksprite;
    cmd_addr[i++] = "--format";
    cmd_addr[i++] = image->format.c_str();
	cmd_addr[i++] = "--dither";
    cmd_addr[i++] = image->dither_algo.c_str();
    cmd_addr[i++] = "--compress";  // don't compress the individual sprite (the sprite itself will be compressed)
    cmd_addr[i++] = "0";
    FILE *image_file = fopen(image->filename.c_str(), "rb");
	if(!image_file) {
		die("Failed to open %s for writing.\n", image->filename.c_str());
	}
    // Start mksprite
    if (subprocess_create(cmd_addr, subprocess_option_no_window|subprocess_option_inherit_environment, &subp) != 0) {
        die("Error: cannot run: %s\n", mksprite);
    }

    // Write PNG to standard input of mksprite
    FILE *mksprite_in = subprocess_stdin(&subp);
	while (1) {
        uint8_t buf[4096];
        int n = fread(buf, 1, sizeof(buf), image_file);
        if (n == 0) break;
		fwrite(buf, 1, n, mksprite_in);
    }
    fclose(mksprite_in); subp.stdin_file = SUBPROCESS_NULL;
	fclose(image_file);
	
    // Read sprite from stdout into memory
    FILE *mksprite_out = subprocess_stdout(&subp);
    while (1) {
        uint8_t buf[4096];
        int n = fread(buf, 1, sizeof(buf), mksprite_out);
        if (n == 0) break;
        fwrite(buf, 1, n, file);
    }

    // Dump mksprite's stderr. Whatever is printed there (if anything) is useful to see
    FILE *mksprite_err = subprocess_stderr(&subp);
    while (1) {
        char buf[4096];
        int n = fread(buf, 1, sizeof(buf), mksprite_err);
        if (n == 0) break;
        fwrite(buf, 1, n, stderr);
    }

    // mksprite should be finished. Extract the return code and abort if failed
    int retcode;
    subprocess_join(&subp, &retcode);
    if (retcode != 0) {
        die("Error: mksprite failed with return code %d\n", retcode);
    }
    subprocess_destroy(&subp);
}

void WriteAnimSpr(const char *path, AnimSprData &data)
{
	fs::path spr_data_path{path};
	spr_data_path.replace_extension(spr_data_path.extension().string()+".dat");
	FILE *file = fopen(path, "wb");
	if(!file) {
		die("Failed to open %s for writing\n", path);
	}
	binwrite_u32(file, 'ASPR');
	binwrite_u32(file, data.anims.size());
	binwrite_u32(file, data.images.size());
	if(!stream_flag) {
		binwrite_symbol_ref(file, "sprdata");
	} else {
		binwrite_u32(file, 0);
	}
	
	for(size_t i=0; i<data.anims.size(); i++) {
		std::string data_name = "animdata" + std::to_string(i);
		binwrite_symbol_ref(file, data_name);
	}
	for(size_t i=0; i<data.anims.size(); i++) {
		std::string data_name = "animdata" + std::to_string(i);
		std::string name = "animname" + std::to_string(i);
		
		binwrite_symbol_set(file, data_name);
		binwrite_symbol_ref(file, name);
		binwrite_u16(file, data.anims[i].frames.size());
		binwrite_u16(file, data.anims[i].total_time);
		for(size_t j=0; j<data.anims[i].frames.size(); j++) {
			binwrite_u16(file, data.anims[i].frames[j].time);
			binwrite_u16(file, data.image_map[data.anims[i].frames[j].image]);
		}
	}
	for(size_t i=0; i<data.anims.size(); i++) {
		std::string name = "animname" + std::to_string(i);
		binwrite_symbol_set(file, name);
		binwrite_string(file, data.anims[i].name.c_str());
	}
	size_t sprdat_maxsize = 0;
	if(stream_flag) {
		fclose(file);
		file = fopen(spr_data_path.c_str(), "wb");
		if(!file) {
			die("Failed to open %s for writing\n", spr_data_path.c_str());
		}
	} else {
		binwrite_align(file, 8);
		binwrite_symbol_set(file, "sprdata");
	}
	
	
	binwrite_symbol_ref(file, "sprdat_maxsize");
	for(size_t i=0; i<data.images.size(); i++) {
		std::string name = "sprite" + std::to_string(i);
		binwrite_symbol_ref(file, name);
	}
	binwrite_symbol_ref(file, "sprdat_end");
	binwrite_align(file, 8);
	for(size_t i=0; i<data.images.size(); i++) {
		std::string name = "sprite" + std::to_string(i);
		size_t data_start = ftell(file);
		binwrite_symbol_set(file, name);
		ConvertImage(file, &data.images[i]);
		binwrite_align(file, 8);
		size_t data_end = ftell(file);
		size_t data_size = data_end-data_start;
		if(data_size > sprdat_maxsize) {
			sprdat_maxsize = data_size;
		}
	}
	binwrite_align(file, 8);
	binwrite_symbol_set(file, "sprdat_end");
	binwrite_symbol_setval(file, sprdat_maxsize, "sprdat_maxsize");
	fclose(file);
}

static char* path_remove_trailing_slash(char *path)
{
    path = strdup(path);
    int n = strlen(path);
    if (path[n-1] == '/' || path[n-1] == '\\')
        path[n-1] = 0;
    return path;
}

// Find the directory where the libdragon tools are installed.
// This is where you can find mksprite, mkfont, etc.
const char *n64_tools_dir(void)
{
    static char *n64_inst = NULL;
    if (n64_inst)
        return n64_inst;

    // Find the tools installation directory.
    n64_inst = getenv("N64_INST");
    if (!n64_inst)
        return NULL;

    // Remove the trailing backslash if any. On some system, running
    // popen with a path containing double backslashes will fail, so
    // we normalize it here.
    n64_inst = path_remove_trailing_slash(n64_inst);
    return n64_inst;
}

void print_args(char *name)
{
    fprintf(stderr, "%s -- Animated sprite builder tool\n\n", name);
    fprintf(stderr, "This tool can be used to compress/decompress arbitrary asset files in a format\n");
    fprintf(stderr, "Usage: %s [flags] <input files...>\n", name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Command-line flags:\n");
    fprintf(stderr, "   -e/--embed				Embed sprites into output\n");
    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		print_args(argv[0]);
		return 1;
	}
	// Find n64 tool directory
	if (!n64_inst) {
		n64_inst = n64_tools_dir();
		if (!n64_inst) {
			die("Error: N64_INST environment variable not set\n");
			return 1;
		}
	}
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
                print_args(argv[0]);
                return 0;
            } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--stream")) {
                stream_flag = true;
            } else {
				die("invalid flag: %s\n", argv[i]);
                return 1;
			}
			continue;
		}
		AnimSprData animspr;
		const char *infn = argv[i];
		const char *outfn;
		if (++i == argc) {
			die("Missing output filename argument\n");
			return 1;
		}
		outfn = argv[i];
		ReadXML(infn, animspr);
		WriteAnimSpr(outfn, animspr);
	}
	
	return 0;
}