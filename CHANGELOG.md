# Changelog

## 1.3.0
- "Tint the tile" now colours the whole tile, edge to edge, as one of the screen's card colours (red, orange,
  yellow, green, blue, purple; the accent takes blue), while the entity is on. Before, a light tint stayed inside the
  card's padding. The icon and the words take the ink on the coloured card. Plugin API 0.26.
- New option "Entity for the tap": with "Switch on a tap", a tap acts on another entity than the one the tile shows. A
  light, switch, cover, fan or input boolean is toggled, a script runs, a scene turns on, a button is pressed. Empty,
  the tap switches the tile's own entity as before. Plugin API 0.15.

## 1.2.2
- For a value or text, "Word when on" is now the name above it, such as "Diesel", and the value stays as it is. The
  `{state}` template of 1.2.0 is gone.

## 1.2.1
- A tile with neither the name nor the state shows its icon without empty labels lying outside it.
- The first release with everything of 1.2.0: the kind "Text or value", the "Icon" choice and "Show the state".

## 1.2.0
- New kind "Text or value": the state as it is, such as a fuel price or a text, without on and off.
- A value or text takes the tile's "Colour when on" (grey on Auto, as before), and its "Word when on" as a template:
  `{state}` stands for the value, "Diesel {state}".
- New option "Icon": one of 43 icons, or Auto, which takes the entity's own icon from Home Assistant when it is one of
  them, else the kind's.
- New toggle "Show the state": off, with "Show the name" off too, the tile shows only its icon.

## 1.1.0
- New tile option "Text size": Normal, Large or Extra large. The name and the state take the screen's larger fonts, and
  the name a darker colour, for large screens such as a 10-inch panel. A long name or state still steps down to fit.

## 1.0.2
- New kinds: temperature and humidity, with a thermometer and a drop; Auto finds them by the device class.
- The picture on the Plugins page shows a door, windows, a light, a washer, a garage, a lock, motion, a temperature
  and a leak sensor.

## 1.0.1
- The icon sits inside its circle as one part, and an idle circle is lighter, so the grey icon reads in the light look too.
- Amber and yellow words and icons take the ink on the light look, where they were too faint to read; the dark look keeps its colours.

## 1.0.0
- First version: one entity per tile as a state, with icons, words and colours by its kind (window, door, light, lock,
  motion and more), chosen on the tile without a build.
