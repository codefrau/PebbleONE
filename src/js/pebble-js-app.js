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
    BATTERY_MODE_ALWAYS   = 2;

var config = {
    seconds_mode: SECONDS_MODE_ALWAYS,
    battery_mode: BATTERY_MODE_IF_LOW,
};

var config_html; // see bottom of file

// read config from persistent storage
Pebble.addEventListener('ready',
    function(e) {
        var json = window.localStorage.getItem('config');
        if (typeof json === 'string')
            config = JSON.parse(json);
        console.log(JSON.stringify(config));
    }
);

// open config window
Pebble.addEventListener('showConfiguration',
    function(e) {
        var html = config_html.replace('__CONFIG__', JSON.stringify(config), 'g');
        Pebble.openURL('data:text/html,' + encodeURI(html + '<!--.html'));
    }
);

// store config and send to watch
Pebble.addEventListener('webviewclosed',
    function(e) {
        if (!e.response) return;
        config = JSON.parse(e.response);
        console.log(JSON.stringify(config));
        window.localStorage.setItem('config', e.response);
        Pebble.sendAppMessage(config,
            function ack(e) { console.log("Successfully delivered message: " + JSON.stringify(e)) },
            function nack(e) { console.log("Unable to deliver message: " + JSON.stringify(e)) });
    }
);


config_html = '<!DOCTYPE html>\
<html>\
<head>\
    <meta name="viewport" content="width=device-width">\
    <style>\
    body {\
        background-color: rgb(100,100,100);\
        font-family: sans-serif;\
    }\
    div,form {\
        text-shadow: 0px 1px 1px white;\
        padding: 10px;\
        margin: 10px 0;\
        border: 1px solid rgb(50,50,50);\
        border-radius: 10px;\
        background: linear-gradient(rgb(230,230,230), rgb(150,150,150));\
    }\
    div.center {text-align: center}\
    h1 {color: rgb(100,100,100); margin-top: 0, padding-top: 0;}\
    input,select {\
        background-color: rgb(128, 255, 0);\
        -webkit-transform: scale(1.8,1.8);\
        -webkit-transform-origin: 0% 0%;\
        width: 155px;\
    }\
    input {\
        float: right;\
        -webkit-transform-origin: 100% 100%;\
    }\
    p,a {color: rgb(200,200,200)}\
    </style>\
</head>\
<body>\
    <div class="center">\
        <h1>ONE</h1>\
    </div>\
    <form onsubmit="return onSubmit(this)">\
        <select id="seconds_mode">\
        <option>Never show seconds</option>\
        <option>No seconds if low battery</option>\
        <option>Always show seconds</option>\
        </select>\
        <br><br><br><br>\
        <select id="battery_mode">\
        <option>Never show battery</option>\
        <option>Only show battery if low</option>\
        <option>Always show battery</option>\
        </select>\
        <br><br><br><br><br>\
        <input type="submit" value="OK">\
        <br><br>\
    </form>\
    <p>\
    This is an open source app: <a href="https://github.com/bertfreudenberg/PebbleONE">Here is the source code</a>.\
    Contributions are highly welcome!<br>\
    -- Bert Freudenberg, December 2013\
    </p>\
    <script>\
        var config = JSON.parse(\'__CONFIG__\');\
        document.getElementById("seconds_mode").options.selectedIndex = config.seconds_mode;\
        document.getElementById("battery_mode").options.selectedIndex = config.battery_mode;\
        function onSubmit(e) {\
            var result = {\
                seconds_mode: document.getElementById("seconds_mode").options.selectedIndex,\
                battery_mode: document.getElementById("battery_mode").options.selectedIndex,\
            };\
            window.location.href = "pebblejs://close#" + JSON.stringify(result);\
            return false;\
        }\
    </script>\
</body>\
</html>';
