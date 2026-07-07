# IncuTEC
Thermoelectric cooling (Peltier stack) based incubator design with gas controll. The temperature controll assembly is inspired by the incubators in the Prakash Lab and expanded to futher uses and gas controll.

The incubator is designed to hold the SQUID microscope (https://github.com/wenzel-lab/SQUID-bioimaging-platform) or optical instruments with a similar footprint. Here you can find an example of such an enclosure https://forum.squid-imaging.org/t/assembly-guide-for-inverted-system-with-the-60-mm-x-60-mm-stage/51. 

In terms of gas controll there are already some open source hardware designs available that can serve as a basis and inspiration. In terms of electronics we should base ourselves on this design that will also form the basis of our anaerobic glove-box in the future https://www.sciencedirect.com/science/article/pii/S2468067221000675. Further inspirations are https://www.sciencedirect.com/science/article/pii/S2468067222000207#f0010

## Wenzel Lab user guide

Start-up procedure:
* Switch on the mains power switch on the cable at the back of the controller - remember that the Raman laser driver is also powered by the same controller/power-supply.
* The temperature set-point and ON/OFF can be set from the menu (white-sub-box with round button)
* Gas valve controll can be activated  "high" and dis-activated "low" from the small slider button in the controller - the gas percentage can only be changed in the microcontroller setting (requires USB connection)
* Open Gas bottle main valve to start CO2 gas flow (never forget to turn off after experiment!)
  
Shut-down procedure
* close gas bottle main valve (no need to touch the regulator)
* vent CO2 with the run valve on the incubator side - located close to the inlet
* switch controller slider button for gas controll to "low"
* turn temperature controller off
* turn off main powersupply switch

Points to clarify:
* temperature ramp speed - controlled by controller digital settings in the interface and by voltage provided to the controller board (12V vs 24V?)
* we need 5% - how far can we currently regulate?
* Buy CO2 sensor for room

## Bill of Materials
* TEC driver/controller http://yexian.com/product/tcm_standard.htm with PID auto-tuning (can be purched from Heidstar)
* Powerful air-air TEC stack with heatsinks and fans https://www.mouser.cl/ProductDetail/Laird-Thermal-Systems/387000612?qs=uwxL4vQweFP51dqtRIyGCw%3D%3D

* The enclosure could be based just on acrylics or on styrofoam (2" thick boxes like these https://www.uline.com/BL_2157/Insulated-Shipping-Kits)

## Design criteria
Initially, the chamber will be designed in two combinable self-sealing parts. A bottom part to incorporate all the feet, cables, etc, and a mobile top part that can be placed over the setup onto the bottom part to seal the chamber. It has a temperature controll that should work precisely at least in the range of 4 degree Celsius to 40 degree celsius (normally it will be used at 37 degree C). Initially we cant to contoll only the Co2 gas addition to normal air to use it for human cell culture and micro-anaeropic pathogens. The chaber shall eb extendable towards an anaerobic solution.
