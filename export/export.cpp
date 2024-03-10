/**************************************************************************/
/*  export.cpp                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                               HOMEBRODOT                               */
/**************************************************************************/
/* Copyright (c) 2023-present Homebrodot contributors.                    */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "export.h"

#include "core/os/os.h"
#include "editor/editor_export.h"
#include "editor/editor_node.h"
#include "editor/editor_settings.h"
#include "platform/wii/logo.gen.h"

class EditorExportPlatformWii : public EditorExportPlatform {
	GDCLASS(EditorExportPlatformWii, EditorExportPlatform);

	int version_code;

	Ref<ImageTexture> logo;

protected:
	virtual void get_export_options(List<ExportOption> *r_options);

public:
	virtual String get_name() const { return "Nintendo Wii"; }
	virtual String get_os_name() const { return "Wii"; }
	virtual Ref<Texture> get_logo() const { return logo; }

	virtual List<String> get_binary_extensions(const Ref<EditorExportPreset> &p_preset) const {
		List<String> list;
		list.push_back("elf");
		list.push_back("dol");
		return list;
	}
	virtual void get_preset_features(const Ref<EditorExportPreset> &p_preset, List<String> *r_features) {}
	virtual void get_platform_features(List<String> *r_features) {}
	virtual void resolve_platform_feature_priorities(const Ref<EditorExportPreset> &p_preset, Set<String> &p_features) {}
	virtual Error export_project(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, int p_flags = 0);

	virtual bool can_export(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates) const;

	EditorExportPlatformWii();
	~EditorExportPlatformWii();
};

void EditorExportPlatformWii::get_export_options(List<ExportOption> *r_options) {
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "custom_template/debug", PROPERTY_HINT_GLOBAL_FILE, "*.elf,*.dol"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "custom_template/release", PROPERTY_HINT_GLOBAL_FILE, "*.elf,*.dol"), ""));

	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/exec_path", PROPERTY_HINT_PLACEHOLDER_TEXT, "apps/your_app/boot.elf"), ""));

	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/name", PROPERTY_HINT_PLACEHOLDER_TEXT, "Game Name"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/coder", PROPERTY_HINT_PLACEHOLDER_TEXT, "Your Name"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/version"), "1"));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/release_date", PROPERTY_HINT_PLACEHOLDER_TEXT, "YYYYmmddHHMMSS"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/short_description"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/long_description"), ""));

	r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/arguments"), ""));
	r_options->push_back(ExportOption(PropertyInfo(Variant::BOOL, "application/ahb_access"), false));

	r_options->push_back(ExportOption(PropertyInfo(Variant::BOOL, "binary_format/embed_pck"), false));
}

bool EditorExportPlatformWii::can_export(const Ref<EditorExportPreset> &p_preset, String &r_error, bool &r_missing_templates) const {
	String err;
	bool valid = false;

	bool dvalid = false;
	bool rvalid = false;

	if (p_preset->get("custom_template/debug") != "") {
		dvalid = FileAccess::exists(p_preset->get("custom_template/debug"));
		if (!dvalid) {
			err += TTR("Custom debug template not found.") + "\n";
		}
	}
	if (p_preset->get("custom_template/release") != "") {
		rvalid = FileAccess::exists(p_preset->get("custom_template/release"));
		if (!rvalid) {
			err += TTR("Custom release template not found.") + "\n";
		}
	}

	valid = dvalid || rvalid;
	r_missing_templates = !valid;

	return valid;
}

Error fixup_embedded_pck(const String &p_path, int64_t p_embedded_start, int64_t p_embedded_size) {
	// Patch the header of the "pck" section in the ELF file so that it corresponds to the embedded data

	if (p_path.ends_with(".dol")) {
		print_error("Cannot patch a .DOL yet! Sorry!");
		return ERR_FILE_CORRUPT; // TODO: Implement .DOL patching
	}

	FileAccess *f = FileAccess::open(p_path, FileAccess::READ_WRITE);
	if (!f) {
		return ERR_CANT_OPEN;
	}

	// Read and check ELF magic number
	{
		uint32_t magic = f->get_32();
		if (magic != 0x464c457f) { // 0x7F + "ELF"
			f->close();
			return ERR_FILE_CORRUPT;
		}
	}

	// Read program architecture bits from class field

	int bits = f->get_8() * 32;

	if (bits == 32 && p_embedded_size >= 0x100000000) {
		f->close();
		ERR_FAIL_V_MSG(ERR_INVALID_DATA, "32-bit executables cannot have embedded data >= 4 GiB.");
	}

	// Get info about the section header table

	f->endian_swap = true; // Wii ELFs are big endian

	int64_t section_table_pos;
	int64_t section_header_size;
	if (bits == 32) {
		section_header_size = 40;
		f->seek(0x20);
		section_table_pos = f->get_32();
		f->seek(0x30);
	} else { // 64
		section_header_size = 64;
		f->seek(0x28);
		section_table_pos = f->get_64();
		f->seek(0x3c);
	}
	int num_sections = f->get_16();
	int string_section_idx = f->get_16();

	// Load the strings table
	uint8_t *strings;
	{
		// Jump to the strings section header
		f->seek(section_table_pos + string_section_idx * section_header_size);

		// Read strings data size and offset
		int64_t string_data_pos;
		int64_t string_data_size;
		if (bits == 32) {
			f->seek(f->get_position() + 0x10);
			string_data_pos = f->get_32();
			string_data_size = f->get_32();
		} else { // 64
			f->seek(f->get_position() + 0x18);
			string_data_pos = f->get_64();
			string_data_size = f->get_64();
		}

		// Read strings data
		f->seek(string_data_pos);
		strings = (uint8_t *)memalloc(string_data_size);
		if (!strings) {
			f->close();
			return ERR_OUT_OF_MEMORY;
		}
		f->get_buffer(strings, string_data_size);
	}

	// Search for the "pck" section

	bool found = false;
	for (int i = 0; i < num_sections; ++i) {
		int64_t section_header_pos = section_table_pos + i * section_header_size;
		f->seek(section_header_pos);

		uint32_t name_offset = f->get_32();
		if (strcmp((char *)strings + name_offset, "pck") == 0) {
			// "pck" section found, let's patch!

			if (bits == 32) {
				f->seek(section_header_pos + 0x10);
				f->store_32(p_embedded_start);
				f->store_32(p_embedded_size);
			} else { // 64
				f->seek(section_header_pos + 0x18);
				f->store_64(p_embedded_start);
				f->store_64(p_embedded_size);
			}

			found = true;
			break;
		}
	}

	memfree(strings);
	f->close();

	return found ? OK : ERR_FILE_CORRUPT;
}

Error EditorExportPlatformWii::export_project(const Ref<EditorExportPreset> &p_preset, bool p_debug, const String &p_path, int p_flags) {
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);

	if (!DirAccess::exists(p_path.get_base_dir())) {
		return ERR_FILE_BAD_PATH;
	}

	// Step 1: Copy export template and icon.png

	String custom_debug = p_preset->get("custom_template/debug");
	String custom_release = p_preset->get("custom_template/release");

	String template_path = p_debug ? custom_debug : custom_release;
	template_path = template_path.strip_edges();

	if (template_path != String() && !FileAccess::exists(template_path)) {
		EditorNode::get_singleton()->show_warning(TTR("Template file not found:") + "\n" + template_path);
		return ERR_FILE_NOT_FOUND;
	}

	if (template_path.ends_with(".dol") && p_preset->get("binary_format/embed_pck")) {
		// TODO: Implement embedding pck into .dol templates
		EditorNode::get_singleton()->show_warning(TTR("\"Embed Pck\" is currently not supported for .DOL templates."));
		return ERR_INVALID_PARAMETER;
	}

	DirAccess *da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	String binname = "boot.";
	binname += template_path.get_extension();
	String actual_path = p_path.get_base_dir() + "/" + binname;
	// TODO: Elf2dol
	Error err = da->copy(template_path, actual_path);
	String icon_path = ProjectSettings::get_singleton()->get("application/config/icon");
	icon_path = icon_path.strip_edges();
	if (err == OK && icon_path != String()) {
		icon_path = ProjectSettings::get_singleton()->globalize_path(icon_path);
		if (da->file_exists(icon_path)) {
			err = da->copy(icon_path, actual_path.get_base_dir() + "/icon.png");
		}
	}
	memdelete(da);

	// Step 2: Save .pck

	if (err == OK) {
		String pck_path;
		if (p_preset->get("binary_format/embed_pck")) {
			pck_path = actual_path;
		} else {
			pck_path = actual_path.get_basename() + ".pck";
		}

		int64_t embedded_pos;
		int64_t embedded_size;
		err = save_pack(p_preset, pck_path, NULL, p_preset->get("binary_format/embed_pck"), &embedded_pos, &embedded_size);
		if (err == OK && p_preset->get("binary_format/embed_pck")) {
			//if(embedded_size >= 0x100000000)
			//{
			// TODO: Determine the size to error out at
			//}

			err = fixup_embedded_pck(actual_path, embedded_pos, embedded_size);
		}
	}

	// Step 3: Write meta.xml

	if (err == OK) {
		FileAccess *meta_xml = FileAccess::open(actual_path.get_base_dir() + "/meta.xml", FileAccess::WRITE, &err);
		if (err == OK) {
			String xml_string = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
			xml_string += "<app version=\"" + (String)(p_preset->get("application/version")) + "\">\n";
			xml_string += "  <name>" + (String)(p_preset->get("application/name")) + "</name>\n";
			xml_string += "  <coder>" + (String)(p_preset->get("application/coder")) + "</coder>\n";
			xml_string += "  <version>" + (String)(p_preset->get("application/version")) + "</version>\n";
			xml_string += "  <release_date>" + (String)(p_preset->get("application/release_date")) + "</release_date>\n";
			xml_string += "  <short_description>" + (String)(p_preset->get("application/short_description")) + "</short_description>\n";
			xml_string += "  <long_description>" + (String)(p_preset->get("application/long_description")) + "</long_description>\n";
			if (p_preset->get("application/ahb_access"))
				xml_string += "  <ahb_access/>\n";

			String args = p_preset->get("application/arguments");
			xml_string += "  <arguments>\n";
			xml_string += "    <arg>" + ((String)p_preset->get("application/exec_path")) + "</arg>\n";
			if (args.strip_edges() != String()) {
				String arg;
				bool in_quote = false;
				bool double_quote = false;
				bool escaping = false;
				for (int i = 0; i < args.size() - 1; i++) {
					char nextchar = args.get(i);
					switch (nextchar) {
						case ' ':
							if (in_quote) {
								if (escaping) {
									arg += "\\";
									escaping = false;
								}
								arg += nextchar;
							} else if (escaping) {
								arg += nextchar;
								escaping = false;
							} else {
								xml_string += "    <arg>" + arg + "</arg>\n";
								arg.clear();
							}
							break;
						case '"':
							if (escaping) {
								arg += nextchar;
								escaping = false;
							} else if (in_quote) {
								if (double_quote)
									in_quote = false;
								else
									arg += nextchar;
							} else {
								in_quote = true;
								double_quote = true;
							}
							break;
						case '\'':
							if (escaping) {
								arg += nextchar;
								escaping = false;
							} else if (in_quote) {
								if (!double_quote)
									in_quote = false;
								else
									arg += nextchar;
							} else {
								in_quote = true;
								double_quote = false;
							}
							break;
						case '\\':
							if (escaping) {
								arg += nextchar;
								escaping = false;
							} else {
								escaping = true;
								continue;
							}
							break;
						default:
							if (escaping) {
								arg += "\\";
								escaping = false;
							}
							arg += nextchar;
							break;
					}
				}
				if (!arg.empty()) {
					if (escaping)
						arg += '\\';

					xml_string += "    <arg>" + arg + "</arg>\n";
					arg.clear();
				}
			}
			xml_string += "  </arguments>\n";
			xml_string += "</app>";

			meta_xml->store_string(xml_string);
			meta_xml->close();
			memdelete(meta_xml);
		}
	}

	return err;
}

EditorExportPlatformWii::EditorExportPlatformWii() {
	Ref<Image> img = memnew(Image(_wii_logo));
	logo.instance();
	logo->create_from_image(img);
}

EditorExportPlatformWii::~EditorExportPlatformWii() {
}

void register_wii_exporter() {
	String exe_ext;
	if (OS::get_singleton()->get_name() == "Windows") {
		exe_ext = "*.exe";
	}

	EDITOR_DEF("export/wii/elf2dol", "");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, "export/wii/elf2dol", PROPERTY_HINT_GLOBAL_FILE, exe_ext));

	Ref<EditorExportPlatformWii> exporter = Ref<EditorExportPlatformWii>(memnew(EditorExportPlatformWii));
	EditorExport::get_singleton()->add_export_platform(exporter);
}
