import unittest
from pathlib import Path

class VietnameseTranslationTests(unittest.TestCase):
    def test_new_translations_exist_in_c_source(self):
        source = Path("payload/vietnamese.c").read_text(encoding="utf-8")
        required_pairs = [
            ('{"First", "Tên"}', "Sort By first name toggle"),
            ('{"Last", "Họ"}', "Sort By last name toggle"),
            ('{"%d of %d", "%d trên %d"}', "Now Playing track counter"),
            ('{"%d of %d Photos", "%d trên %d Ảnh"}', "Photos slideshow counter"),
            ('{"Sort Contacts By First Name", "Sắp xếp danh bạ theo tên"}', "Contacts sort by first"),
            ('{"Sort Contacts By Last Name", "Sắp xếp danh bạ theo họ"}', "Contacts sort by last"),
            ('{"Play Music Library in Sequence", "Phát thư viện nhạc theo thứ tự"}', "Side panel sequence"),
            ('{"Shuffle Entire Music Library", "Xáo trộn toàn bộ thư viện nhạc"}', "Side panel shuffle library"),
            ('{"Shuffle Albums in Playlist", "Xáo trộn các album trong danh sách"}', "Side panel shuffle albums"),
            ('{"Play Current Playlist Once", "Phát danh sách hiện tại một lần"}', "Side panel repeat off"),
            ('{"Repeat Current Song", "Lặp lại bài hát hiện tại"}', "Side panel repeat one"),
            ('{"Repeat Current Playlist", "Lặp lại danh sách phát hiện tại"}', "Side panel repeat all"),
            ('{"Customize Main Menu", "Tùy chỉnh menu chính"}', "Side panel main menu"),
            ('{"Customize Music Menu", "Tùy chỉnh menu nhạc"}', "Side panel music menu"),
            ('{"Set or Lock Volume Limit", "Đặt hoặc khóa giới hạn âm lượng"}', "Side panel volume limit"),
            ('{"Press Play to Lock Volume Limit", "Nhấn Play để khóa giới hạn âm lượng"}', "Side panel lock volume"),
            ('{"Turn Off Sound While Scrolling", "Tắt âm thanh khi cuộn"}', "Side panel clicker off"),
            ('{"Play Sound Through Speaker", "Phát âm thanh qua loa"}', "Side panel clicker speaker"),
            ('{"Play Sound Through Headphones", "Phát âm thanh qua tai nghe"}', "Side panel clicker headphones"),
            ('{"Play Sound Through Headphones and Speaker", "Phát âm qua tai nghe và loa"}', "Side panel clicker both"),
            ('{"Use Original Volume Level", "Dùng mức âm lượng gốc"}', "Side panel sound check off"),
            ('{"Normalize Volume Across All Songs", "Chuẩn hóa âm lượng mọi bài hát"}', "Side panel sound check on"),
            ('{"Adjust Scrolling Speed", "Điều chỉnh tốc độ cuộn"}', "Side panel scroll speed"),
        ]
        for pair, desc in required_pairs:
            with self.subTest(desc=desc):
                self.assertIn(pair, source)

    def test_toggle_values_are_font_safe(self):
        source = Path("payload/vietnamese.c").read_text(encoding="utf-8")
        # Ensure toggle values don't use unsupported compound diacritics
        self.assertIn('{"All", "Tất cả"}', source)
        self.assertIn('{"One", "1 bài"}', source)
        self.assertIn('{"Songs", "Bài hát"}', source)

    def test_font_style_descriptor_patch_declared(self):
        sites = Path("payload/patches/sites.h").read_text()
        patches = Path("payload/patches/patches.c").read_text()
        self.assertIn("OSOS_SETTINGS_STYLE_FONT_WORD", sites)
        self.assertIn("OSOS_SETTINGS_TOGGLE_STYLE_FONT_WORD", sites)
        self.assertIn("OSOS_FONT_RESOURCE_PLAIN_STYLE_WORD", sites)
        self.assertIn("PATCH_WORD(OSOS_SETTINGS_STYLE_FONT_WORD, 0x0dad0020, 0x0dad0021)", patches)
        self.assertIn("PATCH_WORD(OSOS_SETTINGS_TOGGLE_STYLE_FONT_WORD, 0x0dad0020, 0x0dad0021)", patches)
        self.assertIn("PATCH_WORD(OSOS_FONT_RESOURCE_PLAIN_STYLE_WORD, 0x00000000, 0x00000001)", patches)

