var myAPIKey = '772883c4aae75607c2882944e63d570e'; //key for using openweather api
var temperature,night,conditions,todaymax,tommax,forecastarr=[];
var dictionary = {};
var offset = 50; //offset temps by 50 to ensure they're >0 and so can go in a byte array, this is passed back to the C code as KEY_TEMPOFFSET
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
  }                     
);
//callback function for getting current weather, required to ensure that the values are put
// in the dictionary only after the web query has run, if not the webquery which is itself in another
// callback runs asynchronously and so the dictionary data gets set before the data is back from the
// webquery unless the dictionary is set and sent in the webquery callback which prevents getting data
// from multiple queries
var currcallback = function(){
  dictionary.KEY_TEMPERATURE= temperature;
  dictionary.KEY_CONDITIONS= conditions;
};
var forecast3hrcallback = function(){
  dictionary.KEY_TEMPARR = forecastarr;
  dictionary.KEY_ARRLEN = forecastarr.length;
  dictionary.KEY_TEMPOFFSET = offset;
};
var forecastcallback = function(){
  dictionary.KEY_NIGHTTEMP = night;
  dictionary.KEY_TODAYMAX = todaymax;
  dictionary.KEY_TOMMAX = tommax;
  dictionary.KEY_SCALE = Math.max(10,10*Math.ceil(Math.max(night,todaymax,tommax)/10));
  dictionary.KEY_SCALEMIN = Math.min(0,10*Math.floor(Math.min(night,todaymax,tommax)/10));
  // Send to Pebble
  Pebble.sendAppMessage(dictionary,
  function(e) {    console.log('Weather info sent to Pebble successfully!');  },
  function(e) {    console.log('Error sending weather info to Pebble!');  }
  );
};

var xhrRequest = function (url, type, callback,weathercallback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText,weathercallback);
  };
  xhr.open(type, url,false);
  xhr.send();
};

function locationSuccess(pos) {
  //  REQUEST CURRENT WEATHER in metric at the location provided by the phone
  var urlcurr = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
  pos.coords.latitude + '&lon=' + pos.coords.longitude + '&units=metric&appid=' + myAPIKey;
  // This requests forecast weather for the next 2 days in metric (celcius, default is kelvin)
  var url = 'http://api.openweathermap.org/data/2.5/forecast/daily?units=metric&lat=' +
  pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey+'&cnt=2';
  var url3hr = 'http://api.openweathermap.org/data/2.5/forecast?units=metric&lat=' +
  pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey+'&cnt=15';
 //Tests to get Melb values
  // url3hr = 'http://api.openweathermap.org/data/2.5/forecast?units=metric&lat=-37.9072127&lon=145.0418663&appid=' + myAPIKey+'&cnt=15';
  // urlcurr = 'http://api.openweathermap.org/data/2.5/weather?units=metric&lat=-37.9072127&lon=145.0418663&appid=' + myAPIKey+'&cnt=15';
 // url = 'http://api.openweathermap.org/data/2.5/forecast/daily?units=metric&lat=-37.9072127&lon=145.0418663&appid=' + myAPIKey+'&cnt=15';

//  console.log(pos.coords.latitude +"   "+ pos.coords.longitude );
   // Send request to OpenWeatherMap
  
  xhrRequest(urlcurr, 'GET', 
    function(responseText,callback) {
//         console.log("1");

      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);      
       temperature = Math.round(json.main.temp);
      // Conditions
      conditions = json.weather[0].main;  //TMP??
      callback();
      }, currcallback
            );
      xhrRequest(url3hr, 'GET', 
    function(responseText,callback) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      for (var i =0; i<json.list.length ; i++){
       var t = Math.round(json.list[i].main.temp)+offset; //temperature offset by 50 to ensure that values are >0 (and <255) as required by byte array
        forecastarr[i] = t;
//         console.log(t);
      }
      callback();
      },forecast3hrcallback   
  );console.log("2");
  xhrRequest(url, 'GET', 
    function(responseText,callback) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);      
       night = Math.round(json.list[0].temp.night); 
      //var temperature = Math.round(json.list[0].temp.day); 
      todaymax = Math.round(json.list[0].temp.day); 
      tommax =  Math.round(json.list[1].temp.day); 
      callback();
      },forecastcallback   
  );
}
function locationError(err) {
  console.log('Error requesting location!');
}
//get location used to get the weather
function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}
// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }                     
);
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
  }
);