# PUBG-map-hack

This is just my little map hack that I was using for some 300+ hours with no bans whatsoever from BE. I must say BE is pretty uncompetent AC after all, if you just keep yourself at the ring0.

I will update this repo as actively as I can.

YouTube video:

https://www.youtube.com/watch?v=fxbGeqOEhq4

Imgur picture of the web map:

https://imgur.com/y6Isd9F


# HOW DOES IT WORK?

The main principle of this hack is that, the insides of the "usermode app" and "kernel driver" folder are being run on the same machine as the game itself. The usermode app communicates with both the web interface and kernel driver. The web map gets the data from the google's firebase app. So the usermode app updates the firebase app and the web map shows the information stored inside the firebase database.

You can pretty much upload the "online map" insides to any web host. Just remember to update the firebase API keys etc. beforehand, otherwise the map won't simply work.

**REMEMBER TO SET UP THE FIREBASE DATABASE AS "PUBLIC" (so everyone can access/write to it)** More info in the online map folder readme.

This makes it possible to cheat with squads; only one member of the squad must run the cheat program and kernel driver, but everyone can open the map with their own device/browser of choise.

*If you don't want to (or can't) use the firebase, there is also an [alternative version of the map without firebase interface](https://github.com/nakating/PUBG-maphack-map) made by nakating.*

# BUILD INSTRUCTIONS

 - Build kernel driver with Visual Studio 2015. This installation should have WDK included.
 - Build usermode app with Visual Studio 2017. Include the CURL header folder and json folder, also link against the CURL library
 - Upload map, change the firebase values to correspond yours, make this change also in the CURLWrapper.hpp in the usermode app
 - Set firebase as public, see online map folder for more specific instructions.


# Changelog

* 3.11.2017 Updated readme with [nakating](https://github.com/nakating)'s version of the web interface.

* 17.10.2017 Updated with new offsets, also added simple build instructions.

* 12.10.2017 Updated readme

* 10.10.2017 Updated the usermode app. It should work now. The whole solution should work now.

* 10.10.2017 Added a barebone map javascript code. Problem is that much of it is not originally by me, so I stripped it down to very minimum version. If you want to develop it further, please be my guest.

* 10.10.2017 Added kernel driver code. It should work out the box. I used VS2015 and its KMDF tools to dev / compile it.

* 9.10.2017 Added the usermode app barebones. It does not work or compile yet. I will need to work on it for some more hours first.
