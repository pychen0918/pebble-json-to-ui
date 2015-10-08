var testdata = {
  "apptitle": "LightCtrl01",
  "menu": [
    {
      "type": "switch",
      "title": "Turn on/off the light",
      "subtitle": "The living room light",
      "value": 0,
      "id": "switch01"
    },
    {
      "type": "selection",
      "title": "Color selection",
      "subtitle": "We have 3 options",
      "value": 0,
      "id": "selection01",
      "options": [
        {
          "title": "Red",
          "subtitle": "Pure Red"
        },
        {
          "title": "Green",
          "subtitle": "Pure Green"
        },
        {
          "title": "Blue",
          "subtitle": "Pure Blue"
        }
      ]
    }
  ]
};

function sendTestData(){
	var dictionary = {};
	var datastring = "";
	var json = testdata;
	var i;

	dictionary['KEY_APP_TITLE'] = json.apptitle;
	dictionary['KEY_APP_MENU_COUNT'] = json.menu.length;
	for(i in json.menu){
		if(json.menu[i].type == "switch"){
			datastring += "|1|"+json.menu[i].id+"|"+json.menu[i].value+"|"+json.menu[i].title+"|"+json.menu[i].subtitle;
		}
		else if(json.menu[i].type == "selection"){
			var j;
			datastring += "|2|"+json.menu[i].id+"|"+json.menu[i].value+"|"+json.menu[i].title+"|"+json.menu[i].subtitle+"|"+json.menu[i].options.length;
			for(j in json.menu[i].options){
				datastring += "|"+json.menu[i].options[j].title+'|'+json.menu[i].options[j].subtitle;
			}
		}
	}
	dictionary['KEY_APP_DATA'] = datastring;
	console.log("Going to send datastring size: " + dictionary['KEY_APP_DATA'].length + " data: " + dictionary['KEY_APP_DATA']);

	Pebble.sendAppMessage(dictionary,
		function(e) {
			console.log("List sent to Pebble successfully!");
		},  
		function(e) {
			console.log("Error sending list info to Pebble!");
		}
	);
}

Pebble.addEventListener("ready",
	function(e) {
		console.log("Ready");
	}
);

Pebble.addEventListener("appmessage",
	function(e) {
		var req = e.payload['KEY_REQ'];
		console.log("Appmessage received req " + req);
		
		sendTestData();
	}
);
