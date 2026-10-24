/* SPDX-License-Identifier: GPL-3.0-only */
#include "vietnamese.h"
#include "vietnamese_ui.h"
#include "cfw.h"
#include "osos.h"
#include "preferences.h"

enum {
    VIETNAMESE_OFF_RECORD = 0x30454956, /* VIE0 */
    VIETNAMESE_ON_RECORD  = 0x31454956, /* VIE1 */
};

static unsigned int enabled = 1;
static const struct osos_path settings_path = {0, "iPod_Control\\Device\\CFWViet"};

int cfw_vietnamese_enabled(void)
{
    return enabled != 0;
}

void cfw_install_vietnamese(void *bank)
{
    if (!bank)
        return;

    /* Main Menu rows (indices 0..13) */
    static const char *const main_titles_vi[] = {
        "Nhạc",                              /* 0: Music */
        "Video",                             /* 1: Videos */
        "Ảnh",                               /* 2: Photos */
        "Podcasts",                          /* 3: Podcasts */
        "Tiện ích",                          /* 4: Extras */
        "Cài đặt",                           /* 5: Settings */
        "Xáo trộn",                          /* 6: Shuffle Songs */
        "Đang phát",                         /* 7: Now Playing */
        "Nghệ sĩ",                           /* 8: Artists */
        "Album",                             /* 9: Albums */
        "Bài hát",                           /* 10: Songs */
        "Thể loại",                          /* 11: Genres */
        "Soạn giả",                          /* 12: Composers */
        "Sách nói",                          /* 13: Audiobooks */
    };
    static const char *const main_titles_en[] = {
        "Music", "Videos", "Photos", "Podcasts", "Extras", "Settings",
        "Shuffle Songs", "Now Playing", "Artists", "Albums", "Songs",
        "Genres", "Composers", "Audiobooks"
    };

    /* Music Menu rows (indices 0..7) */
    static const char *const music_titles_vi[] = {
        "Cover Flow", "Danh sách phát", "Nghệ sĩ", "Album",
        "Bài hát", "Thể loại", "Soạn giả", "Sách nói"
    };
    static const char *const music_titles_en[] = {
        "Cover Flow", "Playlists", "Artists", "Albums",
        "Songs", "Genres", "Composers", "Audiobooks"
    };

    int is_vi = enabled != 0;
    const char *const *main_titles = is_vi ? main_titles_vi : main_titles_en;
    const char *const *music_titles = is_vi ? music_titles_vi : music_titles_en;

    const uint8_t *main_rows = (const uint8_t *)OSOS_MAIN_MENU_SETTINGS_ROWS;
    for (uint32_t i = 0; i < sizeof(main_titles_vi) / sizeof(main_titles_vi[0]); i++) {
        uint32_t title_id = *(const uint32_t *)(main_rows + i * 20 + 4);
        if (title_id) {
            const char *str = main_titles[i];
            uint32_t len = 0;
            while (str[len]) len++;
            osos_resource_set(bank, 0x53747220, title_id, str, len + 1);
        }
    }

    const uint8_t *music_rows = (const uint8_t *)OSOS_MUSIC_MENU_SETTINGS_ROWS;
    for (uint32_t i = 0; i < sizeof(music_titles_vi) / sizeof(music_titles_vi[0]); i++) {
        uint32_t title_id = *(const uint32_t *)(music_rows + i * 20 + 4);
        if (title_id) {
            const char *str = music_titles[i];
            uint32_t len = 0;
            while (str[len]) len++;
            osos_resource_set(bank, 0x53747220, title_id, str, len + 1);
        }
    }

    /* Common toggle strings: Bật / Tắt or On / Off */
    const char *on_str = is_vi ? "Bật" : "On";
    const char *off_str = is_vi ? "Tắt" : "Off";
    uint32_t on_len = 0, off_len = 0;
    while (on_str[on_len]) on_len++;
    while (off_str[off_len]) off_len++;
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_ON_STRING, on_str, on_len + 1);
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_OFF_STRING, off_str, off_len + 1);
}

void cfw_vietnamese_load(void)
{
    uint32_t record;
    if (cfw_preferences_read(&settings_path, &record, sizeof(record)))
        enabled = (record == VIETNAMESE_ON_RECORD);
    else
        enabled = 1;
    cfw_install_vietnamese(osos_resource_bank());
}

static int save(unsigned int value)
{
    uint32_t record = value ? VIETNAMESE_ON_RECORD : VIETNAMESE_OFF_RECORD;
    return cfw_preferences_write(&settings_path, &record, sizeof(record));
}

int cfw_vietnamese_property(uint32_t type, uint32_t property, uintptr_t *out)
{
    if (type != OSOS_PROPERTY_STRING || property != CFW_Vietnamese_Value)
        return 0;
    *out = osos_settings_on_off(enabled);
    return 1;
}

int cfw_vietnamese_action(void *controller, const char *action, uint32_t argument)
{
    if (!cfw_string_equal(action, "CFW_Vietnamese_Toggle"))
        return 0;
    if (argument == OSOS_ACTION_SUPPORT_QUERY)
        return 1;

    if (save(!enabled)) {
        enabled = !enabled;
        cfw_install_vietnamese(osos_resource_bank());
        osos_model_notify(osos_settings_model(controller), OSOS_NOTIFY_ALL,
                          CFW_Vietnamese_Value);
        osos_ui_invalidate();
    }
    return 1;
}

struct vi_entry {
    const char *en;
    const char *vi;
};

static const struct vi_entry translations[] = {
    /* Main & Music menus */
    {"Music", "Nhạc"},
    {"Videos", "Video"},
    {"Photos", "Ảnh"},
    {"Podcasts", "Podcasts"},
    {"Extras", "Tiện ích"},
    {"Settings", "Cài đặt"},
    {"Shuffle Songs", "Xáo trộn bài"},
    {"Now Playing", "Đang phát"},
    {"Cover Flow", "Cover Flow"},
    {"Playlists", "Danh sách phát"},
    {"Artists", "Nghệ sĩ"},
    {"Albums", "Album"},
    {"Songs", "Bài hát"},
    {"Genres", "Thể loại"},
    {"Composers", "Soạn giả"},
    {"Audiobooks", "Sách nói"},
    {"Search", "Tìm kiếm"},
    {"Album Artists", "Nghệ sĩ album"},
    {"Music Videos", "Video ca nhạc"},
    {"Compilations", "Tuyển tập"},
    {"Compilation", "Tuyển tập"},
    {"Genius", "Genius"},
    {"Genius Mixes", "Bản phối Genius"},
    {"All Songs", "Tất cả bài hát"},
    {"All Albums", "Tất cả album"},
    {"All Artists", "Tất cả nghệ sĩ"},
    {"All Genres", "Tất cả thể loại"},
    {"All Composers", "Tất cả soạn giả"},
    {"On-The-Go", "On-The-Go"},
    {"New Playlist", "Danh sách mới"},
    {"Save Playlist", "Lưu danh sách"},
    {"Clear Playlist", "Xóa danh sách"},

    /* Videos menu */
    {"Movies", "Phim"},
    {"TV Shows", "Truyền hình"},
    {"Video Podcasts", "Video Podcasts"},
    {"Rentals", "Phim thuê"},
    {"Video Playlists", "Danh sách video"},
    {"Video Settings", "Cài đặt video"},

    /* Photos menu */
    {"Photo Library", "Thư viện ảnh"},
    {"All Photos", "Tất cả ảnh"},
    {"Slideshow Settings", "Trình chiếu"},
    {"Time Per Slide", "Thời gian mỗi ảnh"},
    {"Shuffle Photos", "Xáo trộn ảnh"},
    {"Transitions", "Hiệu ứng chuyển"},
    {"Slideshow Music", "Nhạc trình chiếu"},

    /* Settings menu */
    {"About", "Giới thiệu"},
    {"Main Menu", "Menu chính"},
    {"Music Menu", "Menu nhạc"},
    {"Volume Limit", "Giới hạn âm lượng"},
    {"EQ", "Bộ chỉnh âm"},
    {"Sound Check", "Kiểm tra âm thanh"},
    {"Brightness", "Độ sáng"},
    {"Backlight Timer", "Đèn nền"},
    {"Backlight", "Đèn nền"},
    {"Date & Time", "Ngày & Giờ"},
    {"Language", "Ngôn ngữ"},
    {"Legal", "Pháp lý"},
    {"Reset All Settings", "Đặt lại cài đặt"},
    {"Reset Settings", "Đặt lại cài đặt"},
    {"Repeat", "Lặp lại"},
    {"Shuffle", "Xáo trộn"},
    {"Clicker", "Tiếng bấm"},
    {"Dark Mode", "Giao diện tối"},
    {"Sleep Timer", "Hẹn giờ ngủ"},
    {"Sleep", "Hẹn giờ ngủ"},
    {"Sort By", "Sắp xếp theo"},
    {"Scrolling Speed", "Tốc độ cuộn"},

    /* Settings values & toggles */
    {"On", "Bật"},
    {"Off", "Tắt"},
    {"All", "Tất cả"},
    {"One", "Một bài"},
    {"None", "Không"},
    {"Speaker", "Loa"},
    {"Headphones", "Tai nghe"},
    {"Both", "Cả hai"},
    {"2 Seconds", "2 Giây"},
    {"3 Seconds", "3 Giây"},
    {"5 Seconds", "5 Giây"},
    {"10 Seconds", "10 Giây"},
    {"15 Seconds", "15 Giây"},
    {"20 Seconds", "20 Giây"},
    {"30 Seconds", "30 Giây"},
    {"Always On", "Luôn bật"},
    {"Always", "Luôn bật"},
    {"On Keypress", "Khi bấm phím"},
    {"While Playing", "Khi đang phát"},
    {"Ask", "Hỏi"},
    {"Slower Playback", "Phát chậm hơn"},
    {"Normal Playback", "Phát bình thường"},
    {"Faster Playback", "Phát nhanh hơn"},
    {"Slower", "Chậm hơn"},
    {"Normal", "Bình thường"},
    {"Faster", "Nhanh hơn"},

    /* EQ Presets */
    {"Acoustic", "Acoustic"},
    {"Bass Booster", "Tăng Bass"},
    {"Bass Reducer", "Giảm Bass"},
    {"Classical", "Cổ điển"},
    {"Dance", "Dance"},
    {"Deep", "Trầm"},
    {"Electronic", "Điện tử"},
    {"Flat", "Phẳng"},
    {"Hip Hop", "Hip Hop"},
    {"Jazz", "Jazz"},
    {"Latin", "Latin"},
    {"Loudness", "Âm lượng lớn"},
    {"Lounge", "Lounge"},
    {"Piano", "Piano"},
    {"Pop", "Pop"},
    {"R & B", "R&B"},
    {"R&B", "R&B"},
    {"Rock", "Rock"},
    {"Small Speakers", "Loa nhỏ"},
    {"Spoken Word", "Giọng nói"},
    {"Treble Booster", "Tăng Treble"},
    {"Treble Reducer", "Giảm Treble"},
    {"Vocal Booster", "Tăng giọng hát"},
    {"Custom", "Tùy chỉnh"},

    /* About info */
    {"Capacity", "Dung lượng"},
    {"Available", "Khả dụng"},
    {"Version", "Phiên bản"},
    {"Model", "Kiểu máy"},
    {"Format", "Định dạng"},
    {"Serial Number", "Số seri"},
    {"Free", "Trống"},
    {"Used", "Đã dùng"},

    /* Extras menu & submenus */
    {"Clocks", "Đồng hồ"},
    {"Clock", "Đồng hồ"},
    {"Calendar", "Lịch"},
    {"Calendars", "Lịch"},
    {"Contacts", "Danh bạ"},
    {"Notes", "Ghi chú"},
    {"Games", "Trò chơi"},
    {"Stopwatch", "Bấm giờ"},
    {"Screen Lock", "Khóa màn hình"},
    {"Alarms", "Báo thức"},
    {"Alarm", "Báo thức"},
    {"New Alarm", "Báo thức mới"},
    {"Create Alarm", "Tạo báo thức"},
    {"Sound", "Âm thanh"},
    {"Sounds", "Âm thanh"},
    {"Tones", "Nhạc chuông"},
    {"Beep", "Tiếng bíp"},
    {"Snooze", "Báo lại"},
    {"Dismiss", "Bỏ qua"},
    {"Wake up", "Báo thức"},
    {"Start", "Bắt đầu"},
    {"Stop", "Dừng"},
    {"Lap", "Vòng"},
    {"Reset", "Đặt lại"},
    {"Resume", "Tiếp tục"},
    {"Total", "Tổng cộng"},
    {"Shortest", "Ngắn nhất"},
    {"Longest", "Dài nhất"},
    {"Average", "Trung bình"},
    {"Clear Logs", "Xóa nhật ký"},
    {"Voice Memos", "Ghi âm"},
    {"Start Recording", "Bắt đầu ghi"},
    {"Stop and Save", "Dừng và lưu"},
    {"Recordings", "Bản ghi âm"},
    {"Recording", "Đang ghi âm"},

    /* Common actions & prompts */
    {"Yes", "Có"},
    {"No", "Không"},
    {"Cancel", "Hủy"},
    {"Done", "Xong"},
    {"Save", "Lưu"},
    {"OK", "OK"},
    {"Clear", "Xóa"},
    {"Edit", "Sửa"},
    {"Delete", "Xóa"},
    {"Delete All", "Xóa tất cả"},
    {"Play", "Phát"},
    {"Pause", "Tạm dừng"},
    {"Next", "Tiếp"},
    {"Previous", "Trước"},
    {"Back", "Quay lại"},
    {"Continue", "Tiếp tục"},
    {"Lock", "Khóa"},
    {"Unlock", "Mở khóa"},
    {"Loading...", "Đang tải..."},
    {"Please Wait...", "Vui lòng chờ..."},

    /* Battery & Status */
    {"Battery", "Pin"},
    {"Charging", "Đang sạc"},
    {"Charged", "Đã sạc đầy"},
    {"Low Battery", "Pin yếu"},
    {"Connect to Power", "Kết nối sạc"},
    {"Connected", "Đã kết nối"},
    {"Synchronizing", "Đang đồng bộ"},
    {"Do not disconnect.", "Không ngắt kết nối."},
    {"OK to Disconnect", "Có thể ngắt kết nối"},
    {"Eject before disconnecting.", "Đẩy ra trước khi ngắt."},

    /* Date & Time */
    {"Time", "Thời gian"},
    {"Date", "Ngày"},
    {"Hours", "Giờ"},
    {"Minutes", "Phút"},
    {"Seconds", "Giây"},
    {"24-Hour Clock", "Đồng hồ 24 giờ"},
    {"24 Hour Clock", "Đồng hồ 24 giờ"},
    {"Time Zone", "Múi giờ"},
    {"Time in Title", "Giờ ở tiêu đề"},
    {"Today", "Hôm nay"},
    {"Tomorrow", "Ngày mai"},
    {"Yesterday", "Hôm qua"},

    /* Video Settings */
    {"TV Out", "Xuất TV"},
    {"TV Signal", "Tín hiệu TV"},
    {"TV Screen", "Màn hình TV"},
    {"Widescreen", "Màn hình rộng"},
    {"Fit to Screen", "Vừa màn hình"},
    {"Captions", "Phụ đề"},
    {"Subtitles", "Phụ đề"},
    {"Alternate Audio", "Âm thanh phụ"},

    /* CFW options */
    {"Play Next", "Phát tiếp theo"},
    {"Play Last", "Phát cuối cùng"},
    {"Song Info", "Thông tin bài hát"},
};

const char *cfw_vietnamese_translate(const char *s)
{
    if (!s || !*s)
        return 0;
    for (unsigned int i = 0; i < sizeof(translations) / sizeof(translations[0]); i++) {
        if (s[0] == translations[i].en[0] && cfw_string_equal(s, translations[i].en))
            return translations[i].vi;
    }
    return 0;
}
