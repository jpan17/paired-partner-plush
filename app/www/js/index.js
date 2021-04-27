/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

// Wait for the deviceready event before using any of Cordova's device APIs.
// See https://cordova.apache.org/docs/en/latest/cordova/events/events.html#deviceready
document.addEventListener('deviceready', onDeviceReady, false);
var resultAddress = '';
var resultService = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
var resultCharacteristic = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';

function logOutput(...message) {
    document.getElementById('output').innerHTML += message.join(' ') + '<br>';
}

function discoverSuccess(result) {
    logOutput("length:", result.services.length)
    logOutput("Discover returned with status: " + result.status);

    if (result.status === "discovered") {

    // Create a chain of read promises so we don't try to read a property until we've finished
        // reading the previous property.
        // for (let i = 0; i < result.services.length; i++) {
        //     logOutput("<br>===============", JSON.stringify(result.services[i]), "<br>=============<br>")
        // }

        let string = "C";
        let bytes = bluetoothle.stringToBytes(string);
        let encodedString = bluetoothle.bytesToEncodedString(bytes);
        bluetoothle.write(r => logOutput(JSON.stringify(r)), e => logOutput(JSON.stringify(e)), {
            address: resultAddress,
            service: result.services[0].uuid,
            characteristic: result.services[0].characteristics[0].uuid,
            value: encodedString,
        });

        // for (let i = 4; i < result.services.length; i++) {
        //     let service = result.services[i];
        //     for (let j = 1; j < service.characteristics.length; j++) {
        //         let characteristic = service.characteristics[j];
        //         logOutput('test', JSON.stringify({
        //             address: resultAddress,
        //             service: service.uuid,
        //             characteristic: characteristic.uuid,
        //             value: encodedString
        //         }));
        //         bluetoothle.write(r => {}, e => {}, {
        //             address: resultAddress,
        //             service: service.uuid,
        //             characteristic: characteristic.uuid,
        //             value: encodedString
        //         })
        //     }
        // }


    // var readSequence = result.services.reduce(function (sequence, service) {
    //     return sequence.then(function () {
    //         logOutput(result.address)
    //         logOutput("service id:", service.uuid)
    //         logOutput("characteristics:", service.characteristics)
    //         return addService(result.address, service.uuid, service.characteristics);
    //     });

    // }, Promise.resolve());

    // // Once we're done reading all the values, disconnect
    // readSequence.then(function () {

    //     new Promise(function (resolve, reject) {

    //         bluetoothle.disconnect(resolve, reject,
    //             { address: result.address });

    //     }).then(connectSuccess, handleError);

    // });

    }
}

function encodeString(string) {
    let bytes = bluetoothle.stringToBytes(string);
    var data = new Uint8Array(bytes.length + 1);

    let checksum = 0;
    for (let i = 0; i < bytes.length; i++) {
        data[i] = bytes[i]
        checksum += bytes[i]
    }
    checksum = ~checksum;
    checksum %= 256;
    data[bytes.length] = checksum;
    return bluetoothle.bytesToEncodedString(data);
}

function wagglePressed() {
    logOutput("Waggle pressed");
    let encodedString = encodeString("!W")
    bluetoothle.write(r => {}, e => {}, {
        address: resultAddress,
        service: resultService,
        characteristic: resultCharacteristic,
        value: encodedString,
    })

    // logOutput(JSON.stringify({
    //     address: resultAddress,
    //     service: resultService,
    //     characteristic: resultCharacteristic,
    //     value: encodedString,
    // }))
}

function lightPressed() {
    logOutput("Light pressed");
    let encodedString = encodeString("!L")
    bluetoothle.write(r => {}, e => {}, {
        address: resultAddress,
        service: resultService,
        characteristic: resultCharacteristic,
        value: encodedString,
    })

    // logOutput(JSON.stringify({
    //     address: resultAddress,
    //     service: resultService,
    //     characteristic: resultCharacteristic,
    //     value: encodedString,
    // }))
}

function barkPressed() {
    logOutput("Bark pressed");
    let encodedString = encodeString("!B")
    bluetoothle.write(r => {}, e => {}, {
        address: resultAddress,
        service: resultService,
        characteristic: resultCharacteristic,
        value: encodedString,
    })

    // logOutput(JSON.stringify({
    //     address: resultAddress,
    //     service: resultService,
    //     characteristic: resultCharacteristic,
    //     value: encodedString,
    // }))
}

function connectSuccess(result) {

    if (result.status === "connected") {

        logOutput("Connected successfully to: " + result.address);
        resultAddress = result.address;
        bluetoothle.discover(discoverSuccess, e => {}, {
            "address": resultAddress,
            "clearCache": true,
        });
    }
    else if (result.status === "disconnected") {

        logOutput("Disconnected from device: " + result.address);
    }
}

function stopScan() {
    bluetoothle.stopScan(stopScanSuccess, function() {});
}

function startScanSuccess(result) {
    if (result.status === 'scanStarted') {
        logOutput('Scan started');
    }
    if (result.status === 'scanResult') {
        if (result.name === 'Adafruit Bluefruit LE') {
            logOutput('Scan has found a compatible device: ' + result.name + ', ' + result.address);

            stopScan();
            new Promise(function (resolve, reject) {

                bluetoothle.connect(resolve, reject, { address: result.address });
    
            }).then(connectSuccess, function() { logOutput('Failed to connect to bluetooth'); });

        }
    }
}

function stopScanSuccess(result) {
    if (result.status === 'scanStopped') {
        logOutput('Scan stopped');
    } else {
        logOutput('Scan failed to stop');
    }
}

function bluetoothInitializationSuccess(result) {

    if (result.status === 'enabled') {
        logOutput('Bluetooth enabled!');
    } else {
        logOutput('Error enabling Bluetooth');
    }

    // check/request permissions
    bluetoothle.hasPermission(function(result) {
        if (result.hasPermission) {
            logOutput('Has permission: ' + result.hasPermission)            
        } else {
            logOutput('Requesting permission...');
            bluetoothle.requestPermission(function(result) {logOutput('Permission granted: ' + result.requestPermission)}, function() { logOutput('Permission request failed')});
        }
    });
    bluetoothle.isLocationEnabled(function(result) {
        if (result.isLocationEnabled) {
            logOutput('Location enabled: ' + result.isLocationEnabled)            
        } else {
            logOutput('Requesting location...');
            bluetoothle.requestLocation(function(result) {logOutput('Location granted: ' + result.requestLocation)}, function() { logOutput('Location request failed')});
        }
    });

    bluetoothle.startScan(startScanSuccess, function() { logOutput("Scan failed"); }, {
        "services": [],
        "scanMode": bluetoothle.SCAN_MODE_LOW_LATENCY,
        "matchMode": bluetoothle.MATCH_MODE_AGGRESSIVE,
        "matchNum": bluetoothle.MATCH_NUM_MAX_ADVERTISEMENT,
        "callbackType": bluetoothle.CALLBACK_TYPE_ALL_MATCHES,
      });

    setTimeout(stopScan, 10000)

}

function onDeviceReady() {
    // Cordova is now initialized. Have fun!

    console.log('Running cordova-' + cordova.platformId + '@' + cordova.version);
    document.getElementById('deviceready').classList.add('ready');
    logOutput('on device ready');

    // Bluetooth stuff
    new Promise(function (resolve) {

        bluetoothle.initialize(resolve, { request: true, statusReceiver: false });
        logOutput('bluetoothle initialize called');

    }).then(bluetoothInitializationSuccess, function() { logOutput("promise failed for bluetooth init"); });

}
