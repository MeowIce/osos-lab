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

    int is_vi = enabled != 0;

    /* Common toggle strings: Bật / Tắt or On / Off */
    const char *on_str = is_vi ? "Bật" : "On";
    const char *off_str = is_vi ? "Tắt" : "Off";
    uint32_t on_len = 0, off_len = 0;
    while (on_str[on_len]) on_len++;
    while (off_str[off_len]) off_len++;
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_ON_STRING, on_str, on_len + 1);
    osos_resource_set(bank, 0x53747220, OSOS_SETTINGS_OFF_STRING, off_str, off_len + 1);

    /* Date format: %-d %b %Y produces "25 Thg 10 2026" in Vietnamese */
    const char *date_fmt = is_vi ? "%-d %b %Y" : "%b %-d %Y";
    uint32_t fmt_len = 0;
    while (date_fmt[fmt_len]) fmt_len++;
    osos_resource_set(bank, 0x53747220, 0xb3, date_fmt, fmt_len + 1);
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
    {"Shuffle Songs", "Xáo trộn"},
    {"Now Playing", "Đang phát"},
    {"Cover Flow", "Cover Flow"},
    {"Playlists", "Danh sách phát"},
    {"Playlist", "Danh sách phát"},
    {"Artists", "Nghệ sĩ"},
    {"Artist", "Nghệ sĩ"},
    {"Albums", "Album"},
    {"Album", "Album"},
    {"Songs", "Bài hát"},
    {"Song", "Bài hát"},
    {"Genres", "Thể loại"},
    {"Genre", "Thể loại"},
    {"Composers", "Nhà soạn nhạc"},
    {"Composer", "Nhà soạn nhạc"},
    {"Audiobooks", "Sách nói"},
    {"Audiobook", "Sách nói"},
    {"Search", "Tìm kiếm"},
    {"Album Artists", "Nghệ sĩ chính"},
    {"Album Artist", "Nghệ sĩ chính"},
    {"Music Videos", "Video ca nhạc"},
    {"Compilations", "Tuyển tập"},
    {"Compilation", "Tuyển tập"},
    {"Genius", "Genius"},
    {"Genius Mixes", "Bản phối Genius"},
    {"All Songs", "Tất cả bài hát"},
    {"All Albums", "Tất cả album"},
    {"All Artists", "Tất cả nghệ sĩ"},
    {"All Genres", "Tất cả thể loại"},
    {"All Composers", "Tất cả nhà soạn nhạc"},
    {"On-The-Go", "On-The-Go"},
    {"New Playlist", "Danh sách mới"},
    {"Save Playlist", "Lưu danh sách"},
    {"Clear Playlist", "Xóa danh sách"},

    /* Videos menu */
    {"Movies", "Phim"},
    {"Movie", "Phim"},
    {"TV Shows", "Chương trình TV"},
    {"TV Show", "Chương trình TV"},
    {"Video Podcasts", "Video Podcasts"},
    {"Rentals", "Phim thuê"},
    {"Rental", "Phim thuê"},
    {"Video Playlists", "Danh sách video"},
    {"Video Settings", "Cài đặt video"},

    /* Photos menu */
    {"Photo Library", "Thư viện ảnh"},
    {"All Photos", "Tất cả ảnh"},
    {"Slideshow Settings", "Trình chiếu"},
    {"Time Per Slide", "Thời gian ảnh"},
    {"Shuffle Photos", "Xáo trộn ảnh"},
    {"Transitions", "Hiệu ứng"},
    {"Transition", "Hiệu ứng"},
    {"Slideshow Music", "Nhạc nền"},
    {"Wipe across", "Gạt ngang"},
    {"Wipe down", "Gạt xuống"},
    {"Push", "Đẩy"},
    {"Slide", "Trượt"},
    {"Fade", "Mờ dần"},

    /* Settings menu */
    {"About", "Giới thiệu"},
    {"Main Menu", "Menu chính"},
    {"Music Menu", "Menu nhạc"},
    {"Volume Limit", "Giới hạn âm"},
    {"EQ", "Bộ cân bằng"},
    {"Sound Check", "Cân bằng âm"},
    {"Brightness", "Độ sáng"},
    {"Backlight Timer", "Đèn nền"},
    {"Backlight", "Đèn nền"},
    {"Date & Time", "Ngày & Giờ"},
    {"Language", "Ngôn ngữ"},
    {"Legal", "Pháp lý"},
    {"Reset All Settings", "Đặt lại tất cả"},
    {"Reset Settings", "Đặt lại cài đặt"},
    {"Reset Menu", "Đặt lại menu"},
    {"Repeat", "Lặp lại"},
    {"Shuffle", "Xáo trộn"},
    {"Clicker", "Tiếng bấm"},
    {"Dark Mode", "Chế độ tối"},
    {"Sleep Timer", "Hẹn giờ ngủ"},
    {"Sleep", "Ngủ"},
    {"Sort By", "Sắp xếp theo"},
    {"Scrolling Speed", "Tốc độ cuộn"},

    /* Settings values & toggles */
    {"On", "Bật"},
    {"Off", "Tắt"},
    {"All", "Tất cả"},
    {"One", "1 bài"},
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
    {"Slower Playback", "Phát chậm"},
    {"Normal Playback", "Phát chuẩn"},
    {"Faster Playback", "Phát nhanh"},
    {"Slower", "Chậm hơn"},
    {"Normal", "Bình thường"},
    {"Faster", "Nhanh hơn"},

    /* EQ Presets */
    {"Acoustic", "Acoustic"},
    {"Bass Booster", "Tăng âm trầm"},
    {"Bass Reducer", "Giảm âm trầm"},
    {"Classical", "Cổ điển"},
    {"Dance", "Dance"},
    {"Deep", "Trầm sâu"},
    {"Electronic", "Điện tử"},
    {"Flat", "Phẳng"},
    {"Hip Hop", "Hip Hop"},
    {"Jazz", "Jazz"},
    {"Latin", "Latin"},
    {"Loudness", "Độ lớn âm"},
    {"Lounge", "Lounge"},
    {"Piano", "Piano"},
    {"Pop", "Pop"},
    {"R & B", "R&B"},
    {"R&B", "R&B"},
    {"Rock", "Rock"},
    {"Small Speakers", "Loa nhỏ"},
    {"Spoken Word", "Lời nói"},
    {"Treble Booster", "Tăng âm cao"},
    {"Treble Reducer", "Giảm âm cao"},
    {"Vocal Booster", "Tăng giọng"},
    {"Custom", "Tùy chỉnh"},

    /* About info */
    {"Capacity", "Dung lượng"},
    {"Available", "Khả dụng"},
    {"Version", "Phiên bản"},
    {"Model", "Kiểu máy"},
    {"Format", "Định dạng"},
    {"Serial Number", "Số sê-ri"},
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
    {"Tones", "Âm báo"},
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
    {"Start Recording", "Bắt đầu ghi âm"},
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
    {"Next", "Tiếp theo"},
    {"Previous", "Trước"},
    {"Back", "Quay lại"},
    {"Continue", "Tiếp tục"},
    {"Lock", "Khóa"},
    {"Unlock", "Mở khóa"},
    {"Loading...", "Dang tai..."},
    {"Please Wait...", "Vui long cho..."},

    /* USB Disk Mode (pure ASCII font-safe to avoid missing glyphs when drive is locked by USB) */
    {"Connected", "Da ket noi"},
    {"Synchronizing", "Dang dong bo..."},
    {"Do not disconnect.", "Khong ngat ket noi."},
    {"OK to Disconnect", "Co the ngat ket noi"},
    {"Eject before disconnecting.", "Day ra truoc khi ngat ket noi."},
    {"Ejecting", "Dang day ra..."},
    {"You may now disconnect.", "Co the ngat ket noi."},

    /* Battery & Status */
    {"Battery", "Pin"},
    {"Low Battery", "Pin yếu"},
    {"Connect to Power", "Kết nối nguồn điện"},

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
    {"Play Last", "Phát sau cùng"},
    {"Song Info", "Thông tin bài hát"},
    {"CFW Info", "Thông tin CFW"},
    {"Custom EQ", "EQ tùy chỉnh"},
    {"Precut", "Precut"},
    {"Band 1", "Dải 1"},
    {"Band 2", "Dải 2"},
    {"Band 3", "Dải 3"},
    {"Type", "Loại"},
    {"Frequency", "Tần số"},
    {"Gain", "Độ khuếch đại"},
    {"Save settings", "Lưu cài đặt"},
    {"Save failed - retry", "Lưu thất bại - thử lại"},
    {"Invalid filter settings", "Cài đặt bộ lọc không hợp lệ"},

    /* Song Info fields */
    {"Title", "Tiêu đề"},
    {"Year", "Năm"},
    {"Grouping", "Nhóm"},
    {"Duration", "Thời lượng"},
    {"Track", "Bài"},
    {"Disc", "Đĩa"},
    {"Bitrate", "Tốc độ bit"},
    {"File size", "Dung lượng tệp"},
    {"Rating", "Đánh giá"},
    {"Play count", "Lượt phát"},
    {"Skip count", "Lượt bỏ qua"},
    {"Unknown", "Không rõ"},
    {"Unrated", "Chưa đánh giá"},

    /* Extras - Contacts */
    {"No Contacts", "Không có danh bạ"},
    {"No Contact Information", "Không có thông tin danh bạ"},
    {"You can download contacts from iTunes.", "Bạn có thể tải danh bạ từ iTunes."},

    /* Extras - Calendars */
    {"All Calendars", "Tất cả lịch"},
    {"To Do's", "Việc cần làm"},
    {"To Do", "Việc cần làm"},
    {"Loading calendars", "Đang tải lịch..."},
    {"No events for this day", "Không có sự kiện nào"},
    {"You have work to do.", "Bạn có việc cần làm."},
    {"To view your To Do items here, enable syncing from iTunes under the Calendar section of the Contacts tab.", "Để xem việc cần làm tại đây, hãy bật đồng bộ từ iTunes trong mục Lịch của tab Danh bạ."},

    /* Extras - Games */
    {"%d Games", "%d Trò chơi"},
    {"52 Games", "52 Trò chơi"},
    {"No Games", "Không có trò chơi"},

    /* Extras - Screen Lock */
    {"No Combination", "Chưa đặt mã"},
    {"Combination Set", "Đã đặt mã"},
    {"Enter Combination", "Nhập mã khóa"},
    {"New Combination", "Mã khóa mới"},
    {"If you forget the combination, connect to iTunes to unlock your iPod.", "Nếu quên mã khóa, hãy kết nối với iTunes để mở khóa iPod."},

    /* Extras - Stopwatch */
    {"New Timer", "Bấm giờ mới"},

    /* Extras - Alarms & Sleep Timer */
    {"Inactive", "Không hoạt động"},
    {"Active", "Đang hoạt động"},
    {"%d Minutes", "%d Phút"},
    {"15 Minutes", "15 Phút"},
    {"30 Minutes", "30 Phút"},
    {"60 Minutes", "60 Phút"},
    {"90 Minutes", "90 Phút"},
    {"120 Minutes", "120 Phút"},

    /* Extras - World Clock */
    {"Add", "Thêm"},
    {"Region", "Khu vực"},
    {"Africa", "Châu Phi"},
    {"Asia", "Châu Á"},
    {"Atlantic", "Đại Tây Dương"},
    {"Australia", "Châu Úc"},
    {"Europe", "Châu Âu"},
    {"North America", "Bắc Mỹ"},
    {"Pacific", "Thái Bình Dương"},
    {"South America", "Nam Mỹ"},
    {"Americas", "Châu Mỹ"},

    /* Date formats & Days / Months */
    {"%b %-d %Y", "%-d %b %Y"},
    {"%b %-d %y", "%-d %b %y"},
    {"%B %-d, %Y", "%-d %B %Y"},
    {"Jan", "Thg 1"},
    {"Feb", "Thg 2"},
    {"Mar", "Thg 3"},
    {"Apr", "Thg 4"},
    {"May", "Thg 5"},
    {"Jun", "Thg 6"},
    {"Jul", "Thg 7"},
    {"Aug", "Thg 8"},
    {"Sep", "Thg 9"},
    {"Oct", "Thg 10"},
    {"Nov", "Thg 11"},
    {"Dec", "Thg 12"},
    {"January", "Tháng 1"},
    {"February", "Tháng 2"},
    {"March", "Tháng 3"},
    {"April", "Tháng 4"},
    {"June", "Tháng 6"},
    {"July", "Tháng 7"},
    {"August", "Tháng 8"},
    {"September", "Tháng 9"},
    {"October", "Tháng 10"},
    {"November", "Tháng 11"},
    {"December", "Tháng 12"},
    {"Sunday", "Chủ Nhật"},
    {"Monday", "Thứ Hai"},
    {"Tuesday", "Thứ Ba"},
    {"Wednesday", "Thứ Tư"},
    {"Thursday", "Thứ Năm"},
    {"Friday", "Thứ Sáu"},
    {"Saturday", "Thứ Bảy"},
    {"Sun", "CN"},
    {"Mon", "T2"},
    {"Tue", "T3"},
    {"Wed", "T4"},
    {"Thu", "T5"},
    {"Fri", "T6"},
    {"Sat", "T7"},

    /* Right pane descriptions & Empty states */
    {"How fast should audiobooks play?", "Tốc độ phát sách nói?"},
    {"Playback speed will not affect the pitch of the voice in audiobook playback.", "Tốc độ phát sẽ không ảnh hưởng đến cao độ giọng nói."},
    {"Do you want to lock the maximum volume limit?", "Bạn có muốn khóa giới hạn âm lượng tối đa?"},
    {"This will reset all your settings. Your synced content will not be modified.", "Thao tác này sẽ đặt lại tất cả cài đặt. Nội dung đã đồng bộ sẽ không bị thay đổi."},
    {"Do you want to reset the music menu?", "Bạn có muốn đặt lại menu nhạc?"},
    {"This will restore your music menu to its original state.", "Thao tác này sẽ khôi phục menu nhạc về trạng thái ban đầu."},
    {"No Music", "Không có nhạc"},
    {"No Videos", "Không có video"},
    {"No Podcasts", "Không có podcast"},
    {"No Audiobooks", "Không có sách nói"},
    {"No TV Shows", "Không có chương trình TV"},
    {"No Movies", "Không có phim"},
    {"No Music Videos", "Không có video ca nhạc"},
    {"No Photos", "Không có ảnh"},
    {"No Playlists", "Không có danh sách phát"},
    {"No Artists", "Không có nghệ sĩ"},
    {"No Albums", "Không có album"},
    {"No Songs", "Không có bài hát"},
    {"No Genres", "Không có thể loại"},
    {"No Composers", "Không có nhà soạn nhạc"},
    {"No Compilations", "Không có tuyển tập"},
    {"No Rentals", "Không có phim thuê"},
    {"No Notes", "Không có ghi chú"},
    {"No Video", "Không có video"},
    {"No Season", "Không có mùa"},
    {"No Search Results", "Không có kết quả"},
    {"You can download music from iTunes.", "Bạn có thể tải nhạc từ iTunes."},
    {"You can download videos from iTunes.", "Bạn có thể tải video từ iTunes."},
    {"You can download podcasts from iTunes.", "Bạn có thể tải podcast từ iTunes."},
    {"You can download audiobooks from iTunes.", "Bạn có thể tải sách nói từ iTunes."},
    {"You can download TV shows from iTunes.", "Bạn có thể tải chương trình TV từ iTunes."},
    {"You can download movies from iTunes.", "Bạn có thể tải phim từ iTunes."},
    {"You can download music videos from iTunes.", "Bạn có thể tải video ca nhạc từ iTunes."},
    {"You can sync Photos via iTunes.", "Bạn có thể đồng bộ ảnh qua iTunes."},
    {"You can create playlists and sync via iTunes.", "Bạn có thể tạo danh sách phát và đồng bộ qua iTunes."},
    {"You can download rentals from iTunes.", "Bạn có thể tải phim thuê từ iTunes."},
    {"Connect to iTunes to activate Genius.", "Kết nối iTunes để kích hoạt Genius."},
    {"Genius plays songs on your iPod that go great together.  You can start from the Now Playing screen, or choose a song you like and hold down the center button to bring up the Genius option.", "Genius phát các bài hát phù hợp với nhau. Bạn có thể bắt đầu từ màn hình Đang phát hoặc giữ nút chọn trên bài hát để mở Genius."},
    {"Genius Mixes are created for you by iTunes and play songs from your library that go great together. To use Genius Mixes, turn on Genius in iTunes.", "Bản phối Genius phát các bài hát phù hợp trong thư viện của bạn. Hãy bật Genius trong iTunes để sử dụng."},
    {"To view text files here, enable your iPod for disk use in iTunes, then simply drag text files to the Notes folder on your iPod.", "Để xem tệp văn bản tại đây, hãy bật chế độ ổ đĩa trong iTunes rồi kéo tệp vào thư mục Notes."},
    {"Start Genius", "Bắt đầu Genius"},
    {"Creating Genius Playlist", "Đang tạo danh sách Genius..."},
    {"Genius is unavailable for the selected song.", "Genius không khả dụng cho bài hát này."},

    /* Settings, Sort By, Now Playing & Side panel descriptions */
    {"First", "Tên"},
    {"Last", "Họ"},
    {"%d of %d", "%d trên %d"},
    {"%d of %d Photos", "%d trên %d Ảnh"},
    {"Sort Contacts By First Name", "Sắp xếp danh bạ theo tên"},
    {"Sort Contacts By Last Name", "Sắp xếp danh bạ theo họ"},
    {"Play Music Library in Sequence", "Phát thư viện nhạc theo thứ tự"},
    {"Shuffle Entire Music Library", "Xáo trộn toàn bộ thư viện nhạc"},
    {"Shuffle Albums in Playlist", "Xáo trộn các album trong danh sách"},
    {"Play Current Playlist Once", "Phát danh sách hiện tại một lần"},
    {"Repeat Current Song", "Lặp lại bài hát hiện tại"},
    {"Repeat Current Playlist", "Lặp lại danh sách phát hiện tại"},
    {"Customize Main Menu", "Tùy chỉnh menu chính"},
    {"Customize Music Menu", "Tùy chỉnh menu nhạc"},
    {"Set or Lock Volume Limit", "Đặt hoặc khóa giới hạn âm lượng"},
    {"Press Play to Lock Volume Limit", "Nhấn Play để khóa giới hạn âm lượng"},
    {"Turn Off Sound While Scrolling", "Tắt âm thanh khi cuộn"},
    {"Play Sound Through Speaker", "Phát âm thanh qua loa"},
    {"Play Sound Through Headphones", "Phát âm thanh qua tai nghe"},
    {"Play Sound Through Headphones and Speaker", "Phát âm qua tai nghe và loa"},
    {"Use Original Volume Level", "Dùng mức âm lượng gốc"},
    {"Normalize Volume Across All Songs", "Chuẩn hóa âm lượng mọi bài hát"},
    {"Adjust Scrolling Speed", "Điều chỉnh tốc độ cuộn"},
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
