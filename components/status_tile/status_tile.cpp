#include "status_tile.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

namespace esphome::status_tile {

using tessera::Font;
using tessera::Tone;
namespace ui = tessera::ui;

// ---- Readable colours ---------------------------------------------------------------------------------------------

// A colour's luminance and the WCAG contrast of two: amber or yellow words on the light look's white card fall under
// what a person reads, so there they take the ink instead; on the dark look they keep their colour.
static float luminance(lv_color_t c) {
  auto channel = [](uint8_t v) {
    const float s = v / 255.0f;
    return s <= 0.03928f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
  };
  return 0.2126f * channel(c.red) + 0.7152f * channel(c.green) + 0.0722f * channel(c.blue);
}
static bool reads_on_card(lv_color_t c) {
  const float a = luminance(c), b = luminance(ui::color(theme::CARD));
  return (std::max(a, b) + 0.05f) / (std::min(a, b) + 0.05f) >= 3.0f;
}
// A label (words or an icon) in a tone's or a role's colour where that reads on the card, else in the ink.
static void set_readable(lv_obj_t *label, tessera::Tone tone, theme::Role role) {
  if (reads_on_card(ui::tone_color(tone, role))) ui::set_tone(label, tone, role);
  else ui::set_color(label, theme::INK);
}

// ---- Kinds and colours --------------------------------------------------------------------------------------------

// What the entity is: its icons for on and off, its words for on and off, and its colours when the tile leaves them
// on "auto".
enum class Color : uint8_t { AUTO, RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE, ACCENT, GREY };

struct Kind {
  const char *id;
  uint32_t icon_on, icon_off;
  const char *text_on, *text_off;  // keys of the screen's texts
  Color on, off;
};

static const Kind KINDS[] = {
    {"window", 0xF05B1, 0xF11DB, "open", "closed", Color::RED, Color::GREEN},           // window-open / -closed-variant
    {"door", 0xF081C, 0xF081B, "open", "closed", Color::RED, Color::GREEN},             // door-open / door-closed
    {"garage", 0xF06DA, 0xF06D9, "open", "closed", Color::RED, Color::GREEN},           // garage-open / garage
    {"gate", 0xF116A, 0xF0299, "open", "closed", Color::RED, Color::GREEN},             // gate-open / gate
    {"blinds", 0xF111E, 0xF111C, "open", "closed", Color::YELLOW, Color::GREY},         // window-shutter-open / -shutter
    {"light", 0xF0335, 0xF0E4F, "on", "off", Color::YELLOW, Color::GREY},               // lightbulb / lightbulb-off
    {"plug", 0xF06A5, 0xF06A6, "on", "off", Color::ACCENT, Color::GREY},                // power-plug / power-plug-off
    {"motion", 0xF0D91, 0xF1435, "motion", "calm", Color::ORANGE, Color::GREY},         // motion-sensor / -off
    {"presence", 0xF02DC, 0xF06A1, "home", "away", Color::GREEN, Color::GREY},          // home / home-outline
    {"lock", 0xF033F, 0xF033E, "unlocked", "locked", Color::RED, Color::GREEN},         // lock-open / lock
    {"leak", 0xF1502, 0xF058D, "wet", "dry", Color::RED, Color::GREEN},                 // water-alert / water-off
    {"smoke", 0xF192E, 0xF0392, "smoke", "clear", Color::RED, Color::GREEN},            // smoke-detector-alert / -detector
    {"appliance", 0xF072A, 0xF072A, "running", "idle", Color::BLUE, Color::GREY},       // washing-machine
    {"alarm", 0xF099D, 0xF099E, "armed", "disarmed", Color::GREEN, Color::GREY},        // shield-lock / shield-off
    {"switch", 0xF0521, 0xF1A26, "on", "off", Color::ACCENT, Color::GREY},              // toggle-switch / -variant-off
    {"temperature", 0xF050F, 0xF050F, "on", "off", Color::GREY, Color::GREY},         // thermometer
    {"humidity", 0xF058E, 0xF058E, "on", "off", Color::GREY, Color::GREY},            // water-percent
    {"text", 0xF02FD, 0xF02FD, "on", "off", Color::GREY, Color::GREY},                // information-outline
};

// The kind that has no on and off: the state as it is, in the tile's colour and with the tile's own words.
static bool is_text_kind(const Kind *k) { return strcmp(k->id, "text") == 0; }

static const Kind *kind_by_id(const char *id) {
  for (const Kind &k : KINDS)
    if (strcmp(k.id, id) == 0) return &k;
  return nullptr;
}

// "auto": the kind from Home Assistant's device class, else from the entity's domain.
static const Kind *guess_kind(const std::string &device_class, const std::string &entity) {
  static const struct {
    const char *device_class, *kind;
  } BY_CLASS[] = {{"window", "window"}, {"door", "door"},      {"opening", "door"},     {"garage_door", "garage"},
                  {"garage", "garage"}, {"gate", "gate"},      {"shutter", "blinds"},   {"blind", "blinds"},
                  {"curtain", "blinds"}, {"awning", "blinds"}, {"motion", "motion"},    {"occupancy", "motion"},
                  {"presence", "presence"}, {"moisture", "leak"}, {"smoke", "smoke"},   {"gas", "smoke"},
                  {"carbon_monoxide", "smoke"}, {"lock", "lock"}, {"plug", "plug"},     {"outlet", "plug"},
                  {"light", "light"},   {"running", "appliance"}, {"power", "plug"},     {"safety", "alarm"},
                  {"temperature", "temperature"}, {"humidity", "humidity"}, {"moisture_percent", "humidity"}};
  for (const auto &m : BY_CLASS)
    if (device_class == m.device_class) return kind_by_id(m.kind);
  const std::string domain = entity.substr(0, entity.find('.'));
  if (domain == "light") return kind_by_id("light");
  if (domain == "lock") return kind_by_id("lock");
  if (domain == "cover") return kind_by_id("blinds");
  if (domain == "fan" || domain == "switch" || domain == "input_boolean") return kind_by_id("switch");
  return kind_by_id("switch");
}

static Color color_by_id(const char *id) {
  static const char *const IDS[] = {"auto", "red", "orange", "yellow", "green", "blue", "purple", "accent", "grey"};
  for (int i = 0; i < 9; ++i)
    if (strcmp(IDS[i], id) == 0) return static_cast<Color>(i);
  return Color::AUTO;
}

// A colour as the screen's tone and role: red, orange and green follow the screen's alert, busy and good colours.
static void paint(lv_obj_t *obj, Color c, bool fill) {
  Tone tone = Tone::NORMAL;
  theme::Role role = theme::MUTED;
  switch (c) {
    case Color::RED: tone = Tone::ALERT; break;
    case Color::ORANGE: tone = Tone::BUSY; break;
    case Color::GREEN: tone = Tone::GOOD; break;
    case Color::YELLOW: role = theme::MARK_AMBER; break;
    case Color::BLUE: role = theme::MARK_BLUE; break;
    case Color::PURPLE: role = theme::MARK_PURPLE; break;
    case Color::ACCENT: role = theme::ACCENT; break;
    default: role = fill ? theme::TRACK : theme::MUTED; break;
  }
  if (fill) ui::set_fill(obj, tone, role);
  else set_readable(obj, tone, role);
}

// A colour as one of the screen's card colours, for a tile coloured edge to edge (0.26). The accent has no card colour
// of its own: it takes blue. Grey and auto give the card back to what the person chose for it.
static tessera::CardColor card_of(Color c) {
  switch (c) {
    case Color::RED: return tessera::CardColor::RED;
    case Color::ORANGE: return tessera::CardColor::ORANGE;
    case Color::YELLOW: return tessera::CardColor::YELLOW;
    case Color::GREEN: return tessera::CardColor::GREEN;
    case Color::BLUE: return tessera::CardColor::BLUE;
    case Color::PURPLE: return tessera::CardColor::PURPLE;
    case Color::ACCENT: return tessera::CardColor::BLUE;
    default: return tessera::CardColor::NONE;
  }
}

// The states that count as "on": open, on, unlocked, home, wet, running and the like. Everything else is off, or a
// value (a sensor's number) when it is not a state word at all.
static bool is_on(const std::string &s) {
  static const char *const ON[] = {"on", "open", "opening", "unlocked", "unlocking", "home", "playing", "heat",
                                   "detected", "wet", "true", "armed_home", "armed_away", "armed_night",
                                   "armed_vacation", "armed_custom_bypass", "triggered", "jammed"};
  for (const char *o : ON)
    if (s == o) return true;
  return false;
}
static bool is_state_word(const std::string &s) {
  static const char *const OFF[] = {"off", "closed", "closing", "locked", "locking", "not_home", "away", "idle",
                                    "paused", "standby", "dry", "false", "clear", "disarmed", "stopped"};
  if (is_on(s)) return true;
  for (const char *o : OFF)
    if (s == o) return true;
  return false;
}

// ---- Icons --------------------------------------------------------------------------------------------------------

// The icons a tile may choose, by their Material Design Icons name, all of Tessera's set (another one draws nothing).
// The same names find the entity's own icon ("mdi:cash") when Home Assistant gives it one of them.
struct Symbol {
  const char *name;
  uint32_t codepoint;
};

static const Symbol SYMBOLS[] = {
    {"information-outline", 0xF02FD},
    {"form-textbox", 0xF060E},
    {"cash", 0xF0114},
    {"ev-station", 0xF05F1},
    {"car", 0xF010B},
    {"car-electric", 0xF0B6C},
    {"car-battery", 0xF010C},
    {"gauge", 0xF029A},
    {"speedometer", 0xF04C5},
    {"meter-electric", 0xF1A57},
    {"meter-gas", 0xF1A59},
    {"flash", 0xF0241},
    {"solar-power", 0xF0A72},
    {"home-battery", 0xF1901},
    {"battery", 0xF0079},
    {"transmission-tower", 0xF0D3E},
    {"thermometer", 0xF050F},
    {"water-percent", 0xF058E},
    {"water", 0xF058C},
    {"storage-tank", 0xF1A75},
    {"fire", 0xF0238},
    {"snowflake", 0xF0717},
    {"weather-partly-cloudy", 0xF0595},
    {"weather-windy", 0xF059D},
    {"leaf", 0xF032A},
    {"flower", 0xF024A},
    {"trash-can", 0xF0A79},
    {"recycle", 0xF044C},
    {"calendar", 0xF00ED},
    {"clock-outline", 0xF0150},
    {"bell", 0xF009A},
    {"heart", 0xF02D1},
    {"home", 0xF02DC},
    {"account", 0xF0004},
    {"wifi", 0xF05A9},
    {"router-wireless", 0xF0469},
    {"cog", 0xF0493},
    {"package", 0xF03D3},
    {"mailbox", 0xF06EE},
    {"power-plug", 0xF06A5},
    {"lightbulb", 0xF0335},
    {"television", 0xF0502},
    {"music", 0xF075A},
};

// An icon by its name, with or without "mdi:"; 0 for none or one this tile does not know.
static uint32_t symbol_by_name(const char *name) {
  if (strncmp(name, "mdi:", 4) == 0) name += 4;
  if (!*name) return 0;
  for (const Symbol &sym : SYMBOLS)
    if (strcmp(sym.name, name) == 0) return sym.codepoint;
  return 0;
}

// ---- Text sizes ---------------------------------------------------------------------------------------------------

// The tile's text size: the screen's own fonts, a step or two larger, for a large screen seen from across the room.
enum class Size : uint8_t { NORMAL, LARGE, XLARGE };

static Size size_by_id(const char *id) {
  if (strcmp(id, "large") == 0) return Size::LARGE;
  if (strcmp(id, "xlarge") == 0) return Size::XLARGE;
  return Size::NORMAL;
}

// The screen's text fonts, largest first, for stepping down when a text does not fit.
static const Font TEXT_FONTS[] = {Font::HEADLINE, Font::TITLE, Font::BODY_LARGE, Font::BODY};

// The largest font from `start` down to `floor` in which the text fits the room; `floor` when none does.
static Font fitting(const char *text, int room, Font start, Font floor) {
  bool started = false;
  for (Font f : TEXT_FONTS) {
    if (f == start) started = true;
    if (!started) continue;
    if (f == floor || ui::text_width(text, f) <= room) return f;
  }
  return floor;
}

// ---- Drawing helpers ----------------------------------------------------------------------------------------------

static void place(lv_obj_t *obj, int x, int y, int w, int h) {
  lv_obj_set_size(obj, std::max(1, w), std::max(1, h));
  lv_obj_set_pos(obj, x, y);
}

// Opacities written only when they differ: a write redraws the object.
static void set_bg_opa(lv_obj_t *obj, lv_opa_t opa) {
  if (lv_obj_get_style_bg_opa(obj, LV_PART_MAIN) != opa) lv_obj_set_style_bg_opa(obj, opa, 0);
}

// A label inside a circle: a child of it, so the two are one part, centred in it.
static void inside(lv_obj_t *circle, lv_obj_t *label, int w, int h, int dy = 0) {
  lv_obj_set_style_pad_all(circle, 0, 0);
  lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(label, std::max(1, w), std::max(1, h));
  lv_obj_align(label, LV_ALIGN_CENTER, 0, dy);
}

// ---- The tile -----------------------------------------------------------------------------------------------------

// One entity: its icon in a circle of its state's colour, its name, and its state in words (or its value with unit).
// Square or tall, the circle above and the words under it; wide, the circle at the left and the words beside it. The
// whole card takes the colour while the entity is on, when the tile says so.
class StatusTile : public tessera::Tile {
 public:
  explicit StatusTile(const tessera::Plugin *plugin) : plugin_(plugin) {}

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    snprintf(entity_, sizeof entity_, "%s", c.entity ? c.entity : "");
    snprintf(tile_name_, sizeof tile_name_, "%s", c.name ? c.name : "");
    const char *kind = c.options["kind"] | "auto";
    snprintf(kind_id_, sizeof kind_id_, "%s", kind);
    on_color_ = color_by_id(c.options["on_color"] | "auto");
    off_color_ = color_by_id(c.options["off_color"] | "auto");
    snprintf(on_text_, sizeof on_text_, "%s", c.options["on_text"] | "");
    snprintf(off_text_, sizeof off_text_, "%s", c.options["off_text"] | "");
    tint_on_ = c.options["tint"] | true;
    show_name_ = c.options["show_name"] | true;
    show_state_ = c.options["show_state"] | true;
    icon_choice_ = symbol_by_name(c.options["icon"] | "");
    size_ = size_by_id(c.options["text_size"] | "normal");
    switch (size_) {
      case Size::LARGE: name_font_ = Font::BODY_LARGE; state_font_ = Font::HEADLINE; break;
      case Size::XLARGE: name_font_ = Font::TITLE; state_font_ = Font::HEADLINE; break;
      default: name_font_ = Font::BODY; state_font_ = Font::TITLE; break;
    }
    tap_ = c.options["tap"] | false;

    // With neither the name nor the state, the icon alone stands in the middle of the tile.
    wide_ = width_ >= height_ * 3 / 2 && (show_name_ || show_state_);
    // A larger text size on a small tile steps down until the words and the smallest circle fit its height.
    const int min_dia = ui::line_height(Font::ICON_SMALL) + ui::px(8);
    auto text_height = [this]() {
      return (show_name_ ? ui::line_height(name_font_) : 0) + (show_state_ ? ui::line_height(state_font_) : 0);
    };
    auto fits = [&]() {
      return wide_ ? text_height() <= height_ : min_dia + ui::px(6) + text_height() <= height_;
    };
    while (!fits() && (name_font_ != Font::BODY || state_font_ != Font::TITLE)) {
      if (state_font_ == Font::HEADLINE) state_font_ = Font::TITLE;
      else if (name_font_ == Font::TITLE) name_font_ = Font::BODY_LARGE;
      else name_font_ = Font::BODY;
    }
    const int name_h = ui::line_height(name_font_);
    const int state_h = ui::line_height(state_font_);
    const int text_h = (show_name_ ? name_h : 0) + (show_state_ ? state_h : 0);
    if (wide_) {
      dia_ = std::min({ui::mm(14), height_ - ui::px(8), width_ / 3});
    } else {
      dia_ = std::min({ui::mm(16), width_ - ui::px(8), height_ - text_h - ui::px(8)});
    }
    dia_ = std::max(dia_, ui::line_height(Font::ICON_SMALL) + ui::px(8));
    // The large icon where it fits; on the larger text sizes with less room around it, as the words are larger too.
    const int icon_pad = size_ == Size::NORMAL ? ui::px(12) : ui::px(6);
    const Font icon_font = dia_ >= ui::line_height(Font::ICON) + icon_pad ? Font::ICON : Font::ICON_SMALL;
    const int ih = ui::line_height(icon_font);

    circle_ = ui::block(c.parent, theme::TRACK);
    lv_obj_set_style_radius(circle_, LV_RADIUS_CIRCLE, 0);
    icon_ = ui::label(circle_, icon_font, theme::MUTED);
    lv_obj_set_style_text_align(icon_, LV_TEXT_ALIGN_CENTER, 0);
    name_ = ui::label(c.parent, name_font_, name_role());
    state_ = ui::label(c.parent, state_font_, theme::INK);

    if (wide_) {
      const int cy = height_ / 2;
      place(circle_, 0, cy - dia_ / 2, dia_, dia_);
      inside(circle_, icon_, dia_, ih);
      const int x = dia_ + ui::px(12), w = width_ - x, top = cy - text_h / 2;
      place(name_, x, top, w, name_h);
      place(state_, x, top + (show_name_ ? name_h : 0), w, state_h);
    } else {
      const int gap = ui::px(6);
      const int top = std::max(0, (height_ - dia_ - (text_h ? gap : 0) - text_h) / 2);
      place(circle_, (width_ - dia_) / 2, top, dia_, dia_);
      inside(circle_, icon_, dia_, ih);
      lv_obj_set_style_text_align(name_, LV_TEXT_ALIGN_CENTER, 0);
      lv_obj_set_style_text_align(state_, LV_TEXT_ALIGN_CENTER, 0);
      place(name_, 0, top + dia_ + gap, width_, name_h);
      place(state_, 0, top + dia_ + gap + (show_name_ ? name_h : 0), width_, state_h);
    }
    // A label the tile does not show is hidden and kept inside the tile: none of its room lies outside.
    for (auto [label, shown] : {std::pair{name_, show_name_}, std::pair{state_, show_state_}}) {
      if (shown) continue;
      ui::set_text(label, "");
      place(label, 0, 0, 1, 1);
      lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
    paint_now();
  }

  void on_state(JsonObjectConst data) override {
    snprintf(state_text_, sizeof state_text_, "%s", data["state"] | "");
    have_state_ = !data["state"].isNull();
    snprintf(name_text_, sizeof name_text_, "%s", data["name"] | "");
    JsonObjectConst a = data["attributes"];
    snprintf(device_class_, sizeof device_class_, "%s", a["device_class"] | "");
    snprintf(unit_, sizeof unit_, "%s", a["unit_of_measurement"] | "");
    entity_icon_ = symbol_by_name(a["icon"] | "");
    wrong_ = strcmp(data["wait"] | "", "wrong_entity") == 0;
    paint_now();
  }

  void on_theme() override { paint_now(); }

  void on_tap() override {
    if (tap_ && entity_[0]) tessera::action("homeassistant.toggle", entity_);
  }

 private:
  void paint_now() {
    const std::string s = state_text_;
    const Kind *k = strcmp(kind_id_, "auto") == 0 ? guess_kind(device_class_, entity_) : kind_by_id(kind_id_);
    if (!k) k = kind_by_id("switch");
    const bool known = have_state_ && !wrong_ && s != "unavailable" && s != "unknown" && !s.empty();
    // A value (a number, a text) and every state of the kind "text": shown as it is, under the tile's own words.
    const bool value_shown = known && (is_text_kind(k) || !is_state_word(s));

    // The name: for a value, the tile's "word when on" when it has one; else the tile's name, else the entity's.
    const char *title = value_shown && on_text_[0] ? on_text_
                      : tile_name_[0]              ? tile_name_
                      : name_text_[0]              ? name_text_
                                                   : plugin_->text("title");
    if (show_name_) {
      // A long name steps down a size rather than being cut, but never below the screen's own size.
      ui::set_font(name_, fitting(title, text_room(), name_font_, Font::BODY));
      ui::set_text(name_, title);
      ui::set_color(name_, name_role());
    }

    if (!known) {
      // Nothing to show yet: the kind's off icon, grey, and why.
      if (tint_on_) ui::set_card_color(tessera::CardColor::NONE);
      ui::set_fill(circle_, Tone::NORMAL, theme::TRACK);
      set_bg_opa(circle_, LV_OPA_60);
      ui::set_text(icon_, ui::icon(icon_for(k, false)));
      ui::set_color(icon_, theme::MUTED);
      if (show_state_) {
        ui::set_text(state_, plugin_->text(wrong_ ? "wrong" : have_state_ ? "unavailable" : "choose"));
        ui::set_color(state_, theme::MUTED);
      }
      return;
    }

    // A state word (open, on, locked) has its kind's words and colours; a value is shown in the tile's colour when on:
    // grey on Auto.
    const bool word = !value_shown;
    const bool on = word && is_on(s);
    const Color color = !word ? (on_color_ == Color::AUTO ? Color::GREY : on_color_)
                      : on    ? (on_color_ == Color::AUTO ? k->on : on_color_)
                              : (off_color_ == Color::AUTO ? k->off : off_color_);
    const bool colored = color != Color::GREY;
    // A value counts as on for the tint: it is there.
    const bool lit = word ? on : true;
    // The whole card, edge to edge, in the state's colour while it is on, when the tile says so (0.26). The parts on it
    // then take the ink, which reads on every card colour, instead of the state's colour, which would vanish on its own
    // card.
    const bool full = tint_on_ && lit && colored;
    if (tint_on_) ui::set_card_color(full ? card_of(color) : tessera::CardColor::NONE);

    // The circle lightly in the state's colour and the icon fully; grey, a plain circle.
    if (full) {
      ui::set_fill(circle_, Tone::NORMAL, theme::INK);
      set_bg_opa(circle_, LV_OPA_10);
      ui::set_color(icon_, theme::INK);
      if (show_name_) ui::set_color(name_, theme::SLATE);
    } else if (colored) {
      paint(circle_, color, true);
      set_bg_opa(circle_, LV_OPA_20);
      paint(icon_, color, false);
    } else {
      ui::set_fill(circle_, Tone::NORMAL, theme::TRACK);
      set_bg_opa(circle_, LV_OPA_60);
      ui::set_color(icon_, theme::MUTED);
    }
    ui::set_text(icon_, ui::icon(icon_for(k, on)));

    if (!show_state_) return;

    // The words: the tile's own, else the kind's; a value as it is, with its unit.
    std::string text;
    if (!word) {
      std::string value = s;
      if (const char *d = plugin_->text("decimal"); d && *d && *d != '.' && !value.empty() && isdigit(static_cast<unsigned char>(value.back())))
        for (char &ch : value)
          if (ch == '.') ch = *d;
      if (unit_[0]) value += std::string(" ") + unit_;
      text = value;
    } else if (on) {
      text = on_text_[0] ? on_text_ : plugin_->text(k->text_on);
    } else {
      text = off_text_[0] ? off_text_ : plugin_->text(k->text_off);
    }
    // A long word ("Geschlossen") steps down a size on a narrow tile rather than being cut.
    ui::set_font(state_, fitting(text.c_str(), text_room(), state_font_, Font::BODY_LARGE));
    ui::set_text(state_, text);
    if (full) ui::set_color(state_, theme::INK);
    else if (colored) paint(state_, color, false);
    else ui::set_color(state_, word ? theme::INK_SOFT : theme::INK);
  }

  // The icon: the one chosen on the tile, else the entity's own when it is one of Tessera's set, else the kind's.
  uint32_t icon_for(const Kind *k, bool on) const {
    if (icon_choice_) return icon_choice_;
    if (entity_icon_) return entity_icon_;
    return on ? k->icon_on : k->icon_off;
  }

  // The width the words have: beside the circle on a wide tile, the whole tile else.
  int text_room() const { return wide_ ? width_ - dia_ - ui::px(12) : width_; }
  // The name in the muted colour on the normal size, a step darker on the larger ones, to be read from afar.
  theme::Role name_role() const { return size_ == Size::NORMAL ? theme::MUTED : theme::INK_SOFT; }

  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, dia_ = 0;
  bool wide_ = false, tint_on_ = true, show_name_ = true, show_state_ = true, tap_ = false, have_state_ = false;
  bool wrong_ = false;
  uint32_t icon_choice_ = 0, entity_icon_ = 0;
  Color on_color_ = Color::AUTO, off_color_ = Color::AUTO;
  Size size_ = Size::NORMAL;
  Font name_font_ = Font::BODY, state_font_ = Font::TITLE;
  char entity_[96]{}, tile_name_[48]{}, kind_id_[16]{}, on_text_[48]{}, off_text_[48]{};
  char state_text_[64]{}, name_text_[64]{}, device_class_[32]{}, unit_[16]{};
  lv_obj_t *circle_{}, *icon_{}, *name_{}, *state_{};
};

void StatusTilePlugin::setup() {
  add_tile("status", [this]() { return new StatusTile(this); });
}

}  // namespace esphome::status_tile
