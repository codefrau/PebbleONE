/*
 * Copyright (c) 2013 Bert Freudenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// the config dict is sent as app message to the watch
var SECONDS_MODE_NEVER    = 0,
    SECONDS_MODE_IFNOTLOW = 1,
    SECONDS_MODE_ALWAYS   = 2,
    BATTERY_MODE_NEVER    = 0,
    BATTERY_MODE_IF_LOW   = 1,
    BATTERY_MODE_ALWAYS   = 2,
    DATE_MODE_NEVER       = 0,
    DATE_MODE_ALWAYS      = 1,
    BLUETOOTH_MODE_NEVER  = 0,
    BLUETOOTH_MODE_IFOFF  = 1,
    BLUETOOTH_MODE_ALWAYS = 2,
    GRAPHICS_MODE_NORMAL  = 0,
    GRAPHICS_MODE_INVERT  = 1,
    CONNLOST_MODE_IGNORE  = 0,
    CONNLOST_MODE_WARN    = 1;

var config = {
    seconds_mode:   SECONDS_MODE_ALWAYS,
    battery_mode:   BATTERY_MODE_IF_LOW,
    date_mode:      DATE_MODE_ALWAYS,
    bluetooth_mode: BLUETOOTH_MODE_ALWAYS,
    graphics_mode:  GRAPHICS_MODE_NORMAL,
    connlost_mode:  CONNLOST_MODE_IGNORE,
};

// config.html will be included by build process, see build/src/js/pebble-js-app.js
var config_html; 

function send_config_to_pebble() {
    console.log("sending config " + JSON.stringify(config)); 
    Pebble.sendAppMessage(config,
        function ack(e) { console.log("Successfully delivered message " + JSON.stringify(e.data)); },
        function nack(e) { console.log("Unable to deliver message " + JSON.stringify(e)); });
}

// read config from persistent storage
Pebble.addEventListener('ready',
    function () {
        var json = window.localStorage.getItem('config');
        if (typeof json === 'string') {
            config = JSON.parse(json);
            console.log("loaded config " + JSON.stringify(config));
        }
    });

// got message from pebble
Pebble.addEventListener("appmessage",
    function(e) {
        console.log("got message " + JSON.stringify(e.payload));
        if (e.payload.request_config) {
            send_config_to_pebble();
        }
    });

// open config window
Pebble.addEventListener('showConfiguration',
    function () {
        var html = config_html.replace('"REPLACE_ME_AT_RUNTIME"', JSON.stringify(config), 'g');
        Pebble.openURL('data:text/html,' + encodeURIComponent(html + '<!--.html'));
    });

// store config and send to watch
Pebble.addEventListener('webviewclosed',
    function (e) {
        if (e.response && e.response.length) {
            config = JSON.parse(e.response);
            console.log("storing config " + e.response);
            window.localStorage.setItem('config', e.response);
            send_config_to_pebble();
        }
    });

