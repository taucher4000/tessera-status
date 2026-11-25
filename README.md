# Status

Any Home Assistant entity as a state on a tile: a window, a door, a light, a lock, motion, a leak. The icon changes
with the state (an open or closed window, a light on or off), and so does the colour: an open window red, a closed one
green, a light that is on yellow. Use the tile as often as you like, each with its own entity, on any size from
1 x 1 to 4 x 2: square, the circle above and the words under it; wide, the circle beside them.

Everything is set on the tile itself, in the layout's inspector, without a build:

- **Entity**: a binary sensor, sensor, switch, light, input boolean, lock, cover or fan. A group works too, such as
  `binary_sensor.group_windows`, which is on while any window is open.
- **What it is**: window, door, garage door, gate, blinds, light, plug, motion, presence, lock, leak, smoke, appliance,
  alarm, switch, temperature, humidity, or **Text or value** for a state shown as it is (a fuel price, a text). Its
  icons and words. **Auto** takes it from the entity's device class.
- **Icon**: one of 43 icons, such as money, a charging station, a gauge, a tank or a bin. **Auto** takes the entity's
  own icon from Home Assistant when it is one of them, else the kind's.
- **Colour when on** and **Colour when off**: red, orange, yellow, green, blue, purple, the accent, or grey for none.
  **Auto** takes the kind's.
- **Word when on** and **Word when off**: your own, such as "All closed"; empty takes the kind's. For a value or a
  text, **Word when on** is the name above it, such as "Diesel"; the value stays as it is.
- **Tint the tile**: the whole tile takes the colour lightly while the entity is on.
- **Colour the whole tile**: with **Tint the tile** on, the whole card, edge to edge, takes one of the screen's card
  colours while the entity is on, instead of the light tint. The icon and the words then take the ink, so they read on
  it. The accent has no card colour of its own and takes blue.
- **Text size**: Normal, Large or Extra large, for a large screen or one read from across the room. A long name or
  state still steps down a size to fit.
- **Show the name** and **Show the state**: both off, and the tile shows only its icon.
- **Switch on a tap** for a light, a switch or a cover.

A sensor with a number shows it with its unit, in the tile's **Colour when on** (grey on Auto).

## Set up

1. In Tessera, open **Plugins**, **Add with a link**, and paste the link of this repository. Add the plugin to a
   screen and build it once.
2. In **Layout**, place a **Status** tile, choose its entity in the inspector and set what it is and its colours.
   Place as many as you like.
3. For **Switch on a tap**, the screen must be allowed to perform actions (Home Assistant, the ESPHome device's
   options).

## Good to know

- A long entity name: give the tile its own, shorter name in the layout's inspector; the tile takes it instead.
- One entity per tile. For several windows at once, make a group in Home Assistant (Settings, Helpers, Group) and
  choose the group.

## A hobby project

This plugin is a private, non-commercial hobby project. I build and maintain it in my spare time and share it
free of charge as open source under the [MIT License](LICENSE). It is free and will stay free: there is no paid
version, no paid feature and no paid support.

Bug reports and ideas are welcome, but please keep in mind:

- There is no support and no guaranteed response time. I answer when I find the time, which can take days or weeks.
- Not every request gets built. I add what fits the plugin and what I can keep working over time.
- Pull requests are welcome; for a larger change, open an issue first so we can agree on the way.

### No warranty, no liability

The plugin is provided "as is", without any warranty and without guaranteed support, updates or fixes. You use it at
your own risk. To the extent permitted by law, I accept no liability for any damage or data loss resulting from its
use. The full terms are in the [MIT License](LICENSE).

### Buy me a coffee

If the plugin is useful to you and you would like to say thanks, you are welcome to buy me a coffee. A donation is
entirely voluntary: it is a thank-you, not a payment. It does not buy support, features, a response time or any other
service, and the plugin stays exactly the same for everyone, whether they donate or not.

[![Buy me a coffee](https://img.buymeacoffee.com/button-api/?text=Buy%20me%20a%20coffee&emoji=&slug=hflocki&button_colour=FFDD00&font_colour=000000&font_family=Cookie&outline_colour=000000&coffee_colour=ffffff)](https://www.buymeacoffee.com/hflocki)

☕ [buymeacoffee.com/hflocki](https://www.buymeacoffee.com/hflocki)
