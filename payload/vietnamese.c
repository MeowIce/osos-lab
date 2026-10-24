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

void cfw_vietnamese_load(void)
{
    uint32_t record;
    if (cfw_preferences_read(&settings_path, &record, sizeof(record)))
        enabled = (record == VIETNAMESE_ON_RECORD);
    else
        enabled = 1;
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
    *out = (uintptr_t)(enabled ? "Bật" : "Tắt");
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

    /* Settings menu */
    {"About", "Giới thiệu"},
    {"Main Menu", "Menu chính"},
    {"Volume Limit", "Giới hạn âm lượng"},
    {"EQ", "Bộ chỉnh âm"},
    {"Sound Check", "Kiểm tra âm thanh"},
    {"Brightness", "Độ sáng"},
    {"Backlight Timer", "Đèn nền"},
    {"Date & Time", "Ngày & Giờ"},
    {"Language", "Ngôn ngữ"},
    {"Legal", "Pháp lý"},
    {"Reset All Settings", "Đặt lại cài đặt"},
    {"Repeat", "Lặp lại"},
    {"Shuffle", "Xáo trộn"},
    {"Clicker", "Tiếng bấm"},
    {"Dark Mode", "Giao diện tối"},

    /* Extras menu */
    {"Clocks", "Đồng hồ"},
    {"Clock", "Đồng hồ"},
    {"Calendar", "Lịch"},
    {"Contacts", "Danh bạ"},
    {"Notes", "Ghi chú"},
    {"Games", "Trò chơi"},
    {"Stopwatch", "Bấm giờ"},
    {"Screen Lock", "Khóa màn hình"},

    /* Common states / toggles / buttons */
    {"On", "Bật"},
    {"Off", "Tắt"},
    {"All", "Tất cả"},
    {"One", "Một bài"},
    {"None", "Không"},
    {"Speaker", "Loa"},
    {"Headphones", "Tai nghe"},
    {"Both", "Cả hai"},
    {"2 Seconds", "2 Giây"},
    {"5 Seconds", "5 Giây"},
    {"10 Seconds", "10 Giây"},
    {"20 Seconds", "20 Giây"},
    {"Always On", "Luôn bật"},
    {"Yes", "Có"},
    {"No", "Không"},
    {"Cancel", "Hủy"},
    {"Done", "Xong"},
    {"Save", "Lưu"},
    {"OK", "OK"},
    {"Clear", "Xóa"},
    {"Edit", "Sửa"},
    {"Play", "Phát"},
    {"Pause", "Tạm dừng"},
    {"Next", "Tiếp theo"},
    {"Previous", "Trước đó"},
    {"Back", "Quay lại"},
    {"Reset", "Đặt lại"},
    {"Continue", "Tiếp tục"},
    {"Remaining", "Còn lại"},
    {"Elapsed", "Đã qua"},
    {"Track", "Bài"},
    {"Tracks", "Bài hát"},

    /* About info */
    {"Capacity", "Dung lượng"},
    {"Available", "Khả dụng"},
    {"Version", "Phiên bản"},
    {"Model", "Kiểu máy"},
    {"Format", "Định dạng"},
    {"Battery", "Pin"},
    {"Charging", "Đang sạc"},
    {"Charged", "Đã sạc đầy"},

    /* Date & Time / Clock */
    {"Time", "Thời gian"},
    {"Date", "Ngày"},
    {"Hours", "Giờ"},
    {"Minutes", "Phút"},
    {"Seconds", "Giây"},
    {"24-Hour Clock", "Đồng hồ 24 giờ"},
    {"Time Zone", "Múi giờ"},
    {"Alarms", "Báo thức"},
    {"Alarm", "Báo thức"},
    {"Sleep Timer", "Hẹn giờ ngủ"},

    /* CFW options */
    {"Play Next", "Phát tiếp theo"},
    {"Play Last", "Phát cuối cùng"},
    {"Song Info", "Thông tin bài"},
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
