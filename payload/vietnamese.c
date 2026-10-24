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
    *out = (uintptr_t)(enabled ? "Bat" : "Tat");
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
    {"Music", "Nhac"},
    {"Videos", "Video"},
    {"Photos", "Anh"},
    {"Podcasts", "Podcasts"},
    {"Extras", "Tien ich"},
    {"Settings", "Cai dat"},
    {"Shuffle Songs", "Xao tron bai"},
    {"Now Playing", "Dang phat"},
    {"Cover Flow", "Cover Flow"},
    {"Playlists", "Danh sach phat"},
    {"Artists", "Nghe si"},
    {"Albums", "Album"},
    {"Songs", "Bai hat"},
    {"Genres", "The loai"},
    {"Composers", "Soan gia"},
    {"Audiobooks", "Sach noi"},
    {"Search", "Tim kiem"},
    {"Album Artists", "Nghe si album"},
    {"Music Videos", "Video ca nhac"},
    {"Compilations", "Tuyen tap"},
    {"Compilation", "Tuyen tap"},
    {"Genius", "Genius"},
    {"Genius Mixes", "Genius Mixes"},
    {"All Songs", "Tat ca bai hat"},
    {"All Albums", "Tat ca album"},
    {"All Artists", "Tat ca nghe si"},
    {"All Genres", "Tat ca the loai"},
    {"All Composers", "Tat ca soan gia"},
    {"On-The-Go", "On-The-Go"},
    {"New Playlist", "Danh sach moi"},
    {"Save Playlist", "Luu danh sach"},
    {"Clear Playlist", "Xoa danh sach"},

    /* Videos menu */
    {"Movies", "Phim"},
    {"TV Shows", "Truyen hinh"},
    {"Video Podcasts", "Video Podcasts"},
    {"Rentals", "Phim thue"},
    {"Video Playlists", "Danh sach video"},
    {"Video Settings", "Cai dat video"},

    /* Photos menu */
    {"Photo Library", "Thu vien anh"},
    {"All Photos", "Tat ca anh"},
    {"Slideshow Settings", "Trinh chieu"},
    {"Time Per Slide", "Thoi gian moi anh"},
    {"Shuffle Photos", "Xao tron anh"},
    {"Transitions", "Hieu ung chuyen"},
    {"Slideshow Music", "Nhac trinh chieu"},

    /* Settings menu */
    {"About", "Gioi thieu"},
    {"Main Menu", "Menu chinh"},
    {"Music Menu", "Menu nhac"},
    {"Volume Limit", "Gioi han am luong"},
    {"EQ", "Bo chinh am"},
    {"Sound Check", "Kiem tra am thanh"},
    {"Brightness", "Do sang"},
    {"Backlight Timer", "Den nen"},
    {"Backlight", "Den nen"},
    {"Date & Time", "Ngay & Gio"},
    {"Language", "Ngon ngu"},
    {"Legal", "Phap ly"},
    {"Reset All Settings", "Dat lai cai dat"},
    {"Reset Settings", "Dat lai cai dat"},
    {"Repeat", "Lap lai"},
    {"Shuffle", "Xao tron"},
    {"Clicker", "Tieng bam"},
    {"Dark Mode", "Giao dien toi"},
    {"Sleep Timer", "Hen gio ngu"},
    {"Sleep", "Hen gio ngu"},
    {"Sort By", "Sap xep theo"},
    {"Scrolling Speed", "Toc do cuon"},

    /* Settings values & toggles */
    {"On", "Bat"},
    {"Off", "Tat"},
    {"All", "Tat ca"},
    {"One", "Mot bai"},
    {"None", "Khong"},
    {"Speaker", "Loa"},
    {"Headphones", "Tai nghe"},
    {"Both", "Ca hai"},
    {"2 Seconds", "2 Giay"},
    {"3 Seconds", "3 Giay"},
    {"5 Seconds", "5 Giay"},
    {"10 Seconds", "10 Giay"},
    {"15 Seconds", "15 Giay"},
    {"20 Seconds", "20 Giay"},
    {"30 Seconds", "30 Giay"},
    {"Always On", "Luon bat"},
    {"Always", "Luon bat"},
    {"On Keypress", "Khi bam phim"},
    {"While Playing", "Khi dang phat"},
    {"Ask", "Hoi"},
    {"Slower Playback", "Phat cham hon"},
    {"Normal Playback", "Phat binh thuong"},
    {"Faster Playback", "Phat nhanh hon"},
    {"Slower", "Cham hon"},
    {"Normal", "Binh thuong"},
    {"Faster", "Nhanh hon"},

    /* EQ Presets */
    {"Acoustic", "Acoustic"},
    {"Bass Booster", "Tang Bass"},
    {"Bass Reducer", "Giam Bass"},
    {"Classical", "Co dien"},
    {"Dance", "Dance"},
    {"Deep", "Tram"},
    {"Electronic", "Dien tu"},
    {"Flat", "Phang"},
    {"Hip Hop", "Hip Hop"},
    {"Jazz", "Jazz"},
    {"Latin", "Latin"},
    {"Loudness", "Am luong lon"},
    {"Lounge", "Lounge"},
    {"Piano", "Piano"},
    {"Pop", "Pop"},
    {"R & B", "R&B"},
    {"R&B", "R&B"},
    {"Rock", "Rock"},
    {"Small Speakers", "Loa nho"},
    {"Spoken Word", "Giong noi"},
    {"Treble Booster", "Tang Treble"},
    {"Treble Reducer", "Giam Treble"},
    {"Vocal Booster", "Tang giong hat"},
    {"Custom", "Tuy chinh"},

    /* About info */
    {"Capacity", "Dung luong"},
    {"Available", "Kha dung"},
    {"Version", "Phien ban"},
    {"Model", "Kieu may"},
    {"Format", "Dinh dang"},
    {"Serial Number", "So seri"},
    {"Free", "Trong"},
    {"Used", "Da dung"},

    /* Extras menu & submenus */
    {"Clocks", "Dong ho"},
    {"Clock", "Dong ho"},
    {"Calendar", "Lich"},
    {"Calendars", "Lich"},
    {"Contacts", "Danh ba"},
    {"Notes", "Ghi chu"},
    {"Games", "Tro choi"},
    {"Stopwatch", "Bam gio"},
    {"Screen Lock", "Khoa man hinh"},
    {"Alarms", "Bao thuc"},
    {"Alarm", "Bao thuc"},
    {"New Alarm", "Bao thuc moi"},
    {"Create Alarm", "Tao bao thuc"},
    {"Sound", "Am thanh"},
    {"Sounds", "Am thanh"},
    {"Tones", "Nhac chuong"},
    {"Beep", "Tieng bip"},
    {"Snooze", "Bao lai"},
    {"Dismiss", "Bo qua"},
    {"Wake up", "Bao thuc"},
    {"Start", "Bat dau"},
    {"Stop", "Dung"},
    {"Lap", "Vong"},
    {"Reset", "Dat lai"},
    {"Resume", "Tiep tuc"},
    {"Total", "Tong cong"},
    {"Shortest", "Ngan nhat"},
    {"Longest", "Dai nhat"},
    {"Average", "Trung binh"},
    {"Clear Logs", "Xoa nhat ky"},
    {"Voice Memos", "Ghi am"},
    {"Start Recording", "Bat dau ghi"},
    {"Stop and Save", "Dung va luu"},
    {"Recordings", "Ban ghi am"},
    {"Recording", "Dang ghi am"},

    /* Common actions & prompts */
    {"Yes", "Co"},
    {"No", "Khong"},
    {"Cancel", "Huy"},
    {"Done", "Xong"},
    {"Save", "Luu"},
    {"OK", "OK"},
    {"Clear", "Xoa"},
    {"Edit", "Sua"},
    {"Delete", "Xoa"},
    {"Delete All", "Xoa tat ca"},
    {"Play", "Phat"},
    {"Pause", "Tam dung"},
    {"Next", "Tiep"},
    {"Previous", "Truoc"},
    {"Back", "Quay lai"},
    {"Continue", "Tiep tuc"},
    {"Lock", "Khoa"},
    {"Unlock", "Mo khoa"},
    {"Loading...", "Dang tai..."},
    {"Please Wait...", "Vui long cho..."},

    /* Battery & Status */
    {"Battery", "Pin"},
    {"Charging", "Dang sac"},
    {"Charged", "Da sac day"},
    {"Low Battery", "Pin yeu"},
    {"Connect to Power", "Ket noi sac"},
    {"Connected", "Da ket noi"},
    {"Synchronizing", "Dang dong bo"},
    {"Do not disconnect.", "Khong ngat ket noi."},
    {"OK to Disconnect", "Co the ngat ket noi"},
    {"Eject before disconnecting.", "Day ra truoc khi ngat."},

    /* Date & Time */
    {"Time", "Thoi gian"},
    {"Date", "Ngay"},
    {"Hours", "Gio"},
    {"Minutes", "Phut"},
    {"Seconds", "Giay"},
    {"24-Hour Clock", "Dong ho 24 gio"},
    {"24 Hour Clock", "Dong ho 24 gio"},
    {"Time Zone", "Mui gio"},
    {"Time in Title", "Gio o tieu de"},
    {"Today", "Hom nay"},
    {"Tomorrow", "Ngay mai"},
    {"Yesterday", "Hom qua"},

    /* Video Settings */
    {"TV Out", "Xuat TV"},
    {"TV Signal", "Tin hieu TV"},
    {"TV Screen", "Man hinh TV"},
    {"Widescreen", "Man hinh rong"},
    {"Fit to Screen", "Vua man hinh"},
    {"Captions", "Phu de"},
    {"Subtitles", "Phu de"},
    {"Alternate Audio", "Am thanh phu"},

    /* CFW options */
    {"Play Next", "Phat tiep theo"},
    {"Play Last", "Phat cuoi cung"},
    {"Song Info", "Thong tin bai hat"},
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
