# pebble-json-to-ui
Translate json format metadata to Pebble App

## Overview ##

We can turn this:

    {"apptitle":"Light Ctrl","menu":[{"type":"switch","title":"Living room light","subtitle":"Turn on/off the light","value":0,"id":"switch01"},{"type":"selection","title":"Light color","subtitle":"3 options","value":0,"id":"selection01","options":[{"title":"Daylight","subtitle":"Feel Refreshing"},{"title":"Warm","subtitle":"Relaxing"},{"title":"Party","subtitle":"Colorful!"}]}]}

into this:

![](https://raw.githubusercontent.com/pychen0918/pebble-json-to-ui/master/light_01.jpg)
![](https://raw.githubusercontent.com/pychen0918/pebble-json-to-ui/master/light_02.jpg)

The UI is dynamically setup during executing stage.
