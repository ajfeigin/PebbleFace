var myAPIKey = '772883c4aae75607c2882944e63d570e';
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
var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};
function locationSuccess(pos) {
 
  // We will request the weather here
 var url = 'http://api.openweathermap.org/data/2.5/forecast/daily?units=metric&lat=' +
  pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;
  console.log(pos.coords.latitude +"   "+ pos.coords.longitude );
   // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
  console.log(json.city.id);  
      console.log(json.city.name);  
      console.log(json.list[0].temp.day);

      console.log(json.list[1].temp.day);
      console.log(json.list[1].temp.night);
      // Temperature in Kelvin requires adjustment
  //    var temperature = json.list[0].temp.day; //temp.day
      //var temperature = Math.round(json.main.temp);
    //  console.log('Temperature is ' + temperature);

      // Conditions
 //     var conditions = json.weather[0].main;      
   //   console.log('Conditions are ' + conditions);
    }   
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

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
  }
);