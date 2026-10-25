import shutil
import subprocess
import unittest
from pathlib import Path

class LanguageHooksTests(unittest.TestCase):
    def test_language_sites_and_patches_declared(self):
        sites = Path("payload/patches/sites.h").read_text()
        patches = Path("payload/patches/patches.c").read_text()
        
        self.assertIn("#define OSOS_LANGUAGE_COUNT_WORD 0x081e1d9c", sites)
        self.assertIn("#define OSOS_LANGUAGE_NAME_ENTRY 0x081e1ea8", sites)
        self.assertIn("#define OSOS_LANGUAGE_SELECT_CALL 0x0821e380", sites)
        self.assertIn("#define OSOS_LANGUAGE_CURRENT_INDEX_ENTRY 0x081e21e0", sites)

        self.assertIn("PATCH_WORD(OSOS_LANGUAGE_COUNT_WORD, 0xe3a00015, 0xe3a00016);", patches)
        self.assertIn("PATCH_JUMP(OSOS_LANGUAGE_NAME_ENTRY, 0xe92d4010, 0xe1a04001, cfw_language_name);", patches)
        self.assertIn("PATCH_CALL(OSOS_LANGUAGE_SELECT_CALL, OSOS_LANGUAGE_SET, cfw_language_select);", patches)
        self.assertIn("PATCH_JUMP(OSOS_LANGUAGE_CURRENT_INDEX_ENTRY, 0xe92d4010, 0xe5900378, cfw_language_current_index);", patches)

    def test_language_trampoline_and_headers(self):
        sites = Path("payload/patches/sites.h").read_text()
        self.assertIn("#define OSOS_LANGUAGE_SET 0x081e226c", sites)

        lang_h = Path("payload/language.h").read_text()
        self.assertIn("const char *cfw_language_name(void *service, uint32_t index);", lang_h)
        self.assertIn("void cfw_language_select(void *model, uint32_t index);", lang_h)
        self.assertIn("int cfw_language_current_index(void *model);", lang_h)

        vi_h = Path("payload/vietnamese.h").read_text()
        self.assertIn("void cfw_vietnamese_set(int enable);", vi_h)
        self.assertNotIn("cfw_vietnamese_action", vi_h)
        self.assertNotIn("cfw_vietnamese_property", vi_h)

        vi_c = Path("payload/vietnamese.c").read_text()
        self.assertIn("void cfw_vietnamese_set(int enable)", vi_c)
        self.assertNotIn("vietnamese_ui.h", vi_c)

        settings_c = Path("payload/settings.c").read_text()
        self.assertNotIn("cfw_vietnamese_action", settings_c)
        self.assertNotIn("cfw_vietnamese_property", settings_c)

        lang_s = Path("payload/language.S").read_text()
        self.assertIn("cfw_language_name_original:", lang_s)
        self.assertIn("cfw_language_current_index_original:", lang_s)
        self.assertIn("osos_language_set:", lang_s)

    def test_language_utf8_encoding(self):
        lang_c = Path("payload/language.c").read_text()
        expected_utf8 = "Tiếng Việt".encode("utf-8")
        escaped_bytes = b"Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t"
        self.assertEqual(expected_utf8, escaped_bytes)
        self.assertIn('return "Ti\\xe1\\xba\\xbfng Vi\\xe1\\xbb\\x87t";', lang_c)
        self.assertIn("if (index == 18)", lang_c)
        self.assertIn("if (orig >= 18)", lang_c)

    def test_payload_builds_cleanly(self):
        compiler = shutil.which("arm-elf-eabi-gcc")
        if not compiler:
            return
        prefix = compiler[:-3]
        result = subprocess.run(
            ["python3", "tools/build.py", "--cross-prefix", prefix, "osos-recipe"],
            capture_output=True,
            text=True,
            check=True,
        )
        self.assertIn("Built osos-recipe", result.stdout)
