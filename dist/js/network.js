/*
new FakeNetwork({
    frameTime: 1000.0 / 60.0    
    inputSize: 1,
});
*/

class FakeNetwork {

    constructor(config) {
        this.connected = true;

        this.config = config; 

        this.config.playerCount = 1;        

        //this.local = this.config.slot;
        this.local = 0;

        this.networkFrame = -1;
        this.updateFrame = -1;
        this.fastForwardFrame = -1;
        this.bufferedFrame = -1;

        this._needsCopy = false;

        this.inputSize = this.config.inputSize;
        this.frameTime = this.config.frameTime;
        this.tickCounter = 0;

        this.onlaunch = () => {};
        this.ondisconnect = () => {};

        this.inputBuffer = new Uint8Array(this.config.inputSize);
        this.frameBuffer = new Uint8Array(this.config.playerCount * this.config.inputSize + 4);
        // disconnect everyone but the host    
        for (let i = 1; i < this.config.playerCount; ++i) {
            const index = i * this.config.inputSize + 4;
            this.frameBuffer[index] = 0xff;
        }

        setTimeout(() => { 
            this.onlaunch(Math.floor(Math.random() * 0xffff));
        }, 1000);
    }

    close() {
        // do nothing
    }

    ready() {
        // do nothing?
    }

    update(delta) {
        this._needsCopy = false;
        let sent = false;
        this.tickCounter += delta;
        while (this.tickCounter >= this.frameTime) {
            this.tickCounter -= this.frameTime;
            // stagger frame logic
            ++this.bufferedFrame;
            if (this.bufferedFrame % 2 == 0) {
                sent = true;
            }
        }
        return sent;
    }

    needsUpdate() {
        if (this.updateFrame < this.bufferedFrame) {
            ++this.updateFrame;
            if (this.updateFrame % 2 == 0) {
                const index = this.local * this.inputSize + 4;
                this.frameBuffer.set(this.inputBuffer, index);
            }
            this.networkFrame = this.updateFrame;
            this.fastForwardFrame = this.updateFrame;
            this._needsCopy = true;
            return true;
        } else {
            return false;
        }
    }

    needsCopy() { return this._needsCopy; }

    needsFastForward() { return false; }

}

/*
new StaggeredNetwork({
    frameTime: 1000.0 / 60.0,    
    server: this.config.domain.ws,
    local: this.config.domain.slot,
    key: this.config.key,
    playerCount: this.config.slot,
    inputSize: this.config.input,
    //bufferSize: this.config.buffer,
    fastForward: this.config.user.fastForward,
    inputDelay: this.config.user.inputDelay
});
*/

class StaggeredNetwork {

    constructor(config) {
    
        this.config = config; 

        this.onlaunch = () => {};
        this.ondisconnect = () => {};
        
        this.connected = false;
        this.seed = 0;
        this.local = this.config.local;

        // queued frames recieved from server
        this.tempQueue = [];
        this.queue = [];
        this.uploadQueue = [];
        
        // +4 for the frame number
        this.inputBuffer = new Uint8Array(this.config.inputSize);        
        this.sendBuffer = new Uint8Array(this.config.inputSize + 4);
        this.frameBuffer = new Uint8Array(this.config.playerCount * this.config.inputSize + 4);        
        
        console.log(this.sendBuffer);

        this.currentInput = new Uint8Array(this.frameBuffer);

        this.allReady = false;
        this.stalled = false;
        this._needsCopy = false;

        this.frameTime = this.config.frameTime;
        this.inputDelay = this.config.inputDelay;
        this.fastForward = this.config.fastForward;
        this.frameAhead = this.inputDelay + this.fastForward;

        this.networkFrame = -1;
        this.updateFrame = -1;
        this.fastForwardFrame = -1;
        this.bufferedFrame = -1;

        const wsdomain = this.config.server.replace('https://', 'wss://').replace('http://', 'ws://');

        const url = `${wsdomain}ws/${this.config.local}/${this.config.key}`;

        console.log(url);
        this.socket = new WebSocket(url);
        this.socket.binaryType = "arraybuffer";
        
        this.socket.onopen = () => {
            console.log("Opened");
            this.connected = true;
        };

        this.socket.onclose = () => {
            console.log("Closed");
            this.connected = false;
            this.ondisconnect();        
        };

        this.socket.onerror = (error) => {
            console.error("Connection failed:", error);
            this.connected = false;
            this.ondisconnect();        
        };

        this.socket.onmessage = (event) => {
            //console.log("Message from server ", event.data);
            if (event.data instanceof ArrayBuffer) {
                if (event.data.byteLength == 4) {
                    // this is the starting random seed
                    this.seed = new Uint32Array(event.data)[0];
                    this.onlaunch(this.seed);
                } else {
                    //console.log("Recieved: " + event.data.byteLength);
                    this.queue.push(new Uint8Array(event.data));
                    //this.tempQueue.push(new Uint8Array(event.data));
                    //const randomNumber = Math.floor(Math.random() * 5000) + 2000;
                    //setTimeout(() => {
                    //    this.queue.push(this.tempQueue.shift());
                    //}, randomNumber);
                }
            }
        }
        
        this.tickCounter = 0;
    
        return this;
    }

    close() {
        this.socket.close();
    }

    ready() {
        // this should fully fill the delay buffer
        const count = Math.floor(this.inputDelay / 2) + 1;
        this.uploadQueue.push(new Uint8Array(this.inputBuffer));
        for (let i = 1; i < count; ++i) {
            this.sendBuffer[0] = (i & 0x000000ff);
            this.sendBuffer[1] = (i & 0x0000ff00) >> 8;
            this.sendBuffer[2] = (i & 0x00ff0000) >> 16;
            this.sendBuffer[3] = (i & 0xff000000) >> 24;
            this.socket.send(this.sendBuffer);
            this.uploadQueue.push(new Uint8Array(this.inputBuffer));
        }
        this.sendBuffer[0] = 0;
        this.sendBuffer[1] = 0;
        this.sendBuffer[2] = 0;
        this.sendBuffer[3] = 0;
        this.socket.send(this.sendBuffer);
        this.bufferedFrame = this.inputDelay;
        //this.printUpload("READY");
    }
    
    update(delta) {
        this._needsCopy = false;
        let sent = false;
        if (this.queue.length > 0) {
            let buffer = this.queue[this.queue.length - 1];
            let serverFrame = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
            // stagger frame logic
            this.networkFrame = serverFrame + serverFrame + 1;
            this.allReady = true;
        }
        if (this.allReady == true) {
            if (this.stalled == false) {
                this.tickCounter += delta;
            }
            this.stalled = false;
            while (this.tickCounter >= this.frameTime && this.stalled == false) {
                if (this.bufferedFrame - this.networkFrame + 1 <= this.frameAhead) {
                    this.tickCounter -= this.frameTime;
                    // stagger frame logic
                    ++this.bufferedFrame;
                    if (this.bufferedFrame % 2 == 0) {
                        const sentFrame = Math.floor(this.bufferedFrame / 2);
                        this.uploadQueue.push(new Uint8Array(this.inputBuffer));
                        //this.printUpload("SENDING");
                        this.sendBuffer[0] = (sentFrame & 0x000000ff);
                        this.sendBuffer[1] = (sentFrame & 0x0000ff00) >> 8;
                        this.sendBuffer[2] = (sentFrame & 0x00ff0000) >> 16;
                        this.sendBuffer[3] = (sentFrame & 0xff000000) >> 24;
                        this.sendBuffer.set(this.inputBuffer, 4);
                        this.socket.send(this.sendBuffer);
                        sent = true;
                    }
                    this.stalled = false;
                } else {
                    this.stalled = true;
                }            
            }
        }
        return sent;
    }

    needsUpdate() {
        if (this.updateFrame < this.bufferedFrame - this.inputDelay && this.updateFrame < this.networkFrame) {
            // stagger frame logic
            ++this.updateFrame;
            if (this.updateFrame % 2 == 0) {
                this.currentInput = this.queue.shift();
                this.uploadQueue.shift();

                this.frameBuffer.set(this.currentInput, 0); 

                //console.log(this.currentInput);

                //this.printUpload("SHIFTING");
            }
            this.fastForwardFrame = this.updateFrame;
            this._needsCopy = true;
            return true;
        } else {
            return false;
        }
    }

    needsCopy() {
        if (this._needsCopy) {
            //this.frameBuffer.set(this.currentInput, 0); 
            return true;
        } else {
            return false;
        }
    }

    needsFastForward() {
        if (this.fastForwardFrame < this.bufferedFrame - this.inputDelay) {
            ++this.fastForwardFrame;
            // insert local input into local slot of frameBuffer
            const index = this.local * this.inputSize + 4;
            
            const uploadIndex = Math.floor((this.fastForwardFrame - this.updateFrame - 1) / 2);
            //const frame = new Uint32Array(this.uploadQueue[uploadIndex].buffer)[0];
            //console.log("Queued Input: ", uploadIndex, "Frame: ", frame);
            //console.log("Queued Input: ", uploadIndex);
            //this.printUpload("FAST FORWARD");
            
            this.frameBuffer.set(this.uploadQueue[uploadIndex], index);

            return true;
        } else {
            return false;
        }
    }

};