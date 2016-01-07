var myAPIKey = '772883c4aae75607c2882944e63d570e';
var temperature,night,conditions;
var dictionary = {};
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
   console.log("111 temp is  " + temperature);
  //dictionary.push({
  dictionary.KEY_TEMPERATURE= temperature;
  dictionary.KEY_CONDITIONS= conditions;
  //});
  console.log("123" + dictionary.KEY_CONDITIONS);
};

var forecastcallback = function(){
   console.log("112 night temp is  " + night);
  dictionary.KEY_NIGHTTEMP = night;
  console.log("124 " + dictionary.KEY_NIGHTTEMP);
  // Send to Pebble
Pebble.sendAppMessage(dictionary,
  function(e) {
    console.log('Weather info sent to Pebble successfully!');
  },
  function(e) {
    console.log('Error sending weather info to Pebble!');
  }
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
 // console.log(pos.coords.latitude +"   "+ pos.coords.longitude );
   // Send request to OpenWeatherMap
  
  xhrRequest(urlcurr, 'GET', 
    function(responseText,callback) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);      
       temperature = Math.round(json.main.temp);
      //var temperature = Math.round(json.list[0].temp.day); 
//      console.log('Temperature is ' + temperature);

      // Conditions
      conditions = json.weather[0].main;  //TMP??
      callback();
      }, currcallback
  );
  xhrRequest(url, 'GET', 
    function(responseText,callback) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);      
       night = json.list[0].temp.night; 
      //var temperature = Math.round(json.list[0].temp.day); 

      callback();
      },forecastcallback   
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

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