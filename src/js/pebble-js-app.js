var testdata = {
  "apptitle": "Light Ctrl",
  "menu": [
    {
      "type": "switch",
      "title": "Living room light",
      "subtitle": "Turn on/off the light",
      "value": 0,
      "id": "switch01"
    },
    {
      "type": "selection",
      "title": "Light color",
      "subtitle": "3 options",
      "value": 0,
      "id": "selection01",
      "options": [
        {
          "title": "Daylight",
          "subtitle": "Feel Refreshing"
        },
        {
          "title": "Warm",
          "subtitle": "Relaxing"
        },
        {
          "title": "Party",
          "subtitle": "Colorful!"
        }
      ]
    }
  ]
};

function send_test_data(){
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

function update_value(id, value){
	var i;
	var json = testdata;

	console.log("id = "+id+" value = "+value);
	
	for(i in json.menu){
		if(json.menu[i].id == id){
			json.menu[i].value = value;
		}
	}
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
		if(req == 0)  // ask app list
			send_test_data();
		else if(req == 1) // update id:value
			update_value(e.payload['KEY_ID'], e.payload['KEY_VALUE']);
		else
			console.log("Error: unknown req:"+req);
	}
);
