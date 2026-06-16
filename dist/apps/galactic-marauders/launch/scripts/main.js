// main game class
class Main {

    async init(config) {
        // initialize pixi.js
        this.pixi = new PIXI.Application();
        await this.pixi.init({ background: '#000000', resizeTo: window });
        document.body.appendChild(this.pixi.canvas);
        PIXI.TextureStyle.defaultOptions.scaleMode = 'nearest';					

        this.game = new Module.BindGame();

        this.texture = undefined;
        PIXI.Assets.load('./images/invaders-sprite.json').then((texture) => {
            this.texture = texture;
        });
        this.renderObjectIndex = 0;

        this.config = config;

        // container for game objects
        this.container = new PIXI.Container();
        this.container.virtualWidth = 960 * 2;
        this.container.virtualHeight = 540 * 2;
        this.pixi.stage.addChild(this.container);

        this.debugTouchIndex = 0;
        this.debugTouch = new PIXI.Container();
        this.pixi.stage.addChild(this.debugTouch);

        this.visualizer = new NetworkVisualizer();
        this.visualizer.visible = false;
        this.pixi.stage.addChild(this.visualizer);

        // system display text
        this.displayText = new PIXI.Text({
            text: 'Loading...',
            style: {
                fill: '#ffffff',
                fontSize: 24
            }
        });
        this.pixi.stage.addChild(this.displayText);

        // event listeners
        this.resize();
        window.addEventListener("resize", () => this.resize());

        window.addEventListener('touchstart', (e) => e.preventDefault());
        window.addEventListener('touchend', (e) => e.preventDefault());

        this.pointerList = {};
        window.onpointerdown = (e) => {
            this.pointerList[e.pointerId] = {
                x: e.clientX,
                y: e.clientY
            };
        };
        window.onpointerup = (e) => {
            delete this.pointerList[e.pointerId];
        };
        window.onpointermove = (e) => {
            const id = e.pointerId;
            if (id in this.pointerList) {
                this.pointerList[id].x = e.clientX;
                this.pointerList[id].y = e.clientY;
            }
        };

        this.keys = {
            left: false,
            right: false,
            primary: false
        }
        this.keyLeft = false;
        this.keyRight = false;
        window.addEventListener('keydown', (event) => {
            if (event.key == '`') {
                if (this.pixi.canvas.requestFullscreen) {
                    this.pixi.canvas.requestFullscreen();
                } else if (this.pixi.canvas.webkitRequestFullscreen) { // Safari
                    this.pixi.canvas.webkitRequestFullscreen();
                } else if (this.pixi.canvas.msRequestFullscreen) { // IE11
                    this.pixi.canvas.msRequestFullscreen();
                }
            } else if (event.key == 'ArrowLeft') {
                this.keyLeft = true;
            } else if (event.key == 'ArrowRight') {
                this.keyRight = true;
            } else if (event.key == ' ') {
                this.paused = !this.paused;
            } else if (event.key == '\\') {
                this.visualizer.visible = !this.visualizer.visible;
            }
        });
        window.addEventListener('keyup', (event) => {
            if (event.key == 'ArrowLeft') {
                this.keyLeft = false;
            } else if (event.key == 'ArrowRight') {
                this.keyRight = false;
            }
        });

        // initalize network config
        this.displayText.text = 'Connecting to Relay Server...';
        
        if (this.config.slot > 1) {        
            this.network = new StaggeredNetwork({
                frameTime: 1000.0 / 60.0,
                server: this.config.domain.url,
                local: this.config.domain.slot,
                key: this.config.domain.key,
                playerCount: this.config.slot,
                inputSize: this.config.input,
                fastForward: this.config.user.fastForward,
                inputDelay: this.config.user.inputDelay
            });
        } else {
            this.network = new FakeNetwork({
                frameTime: 1000.0 / 60.0,
                inputSize: 1
            });
        }

        this.network.ondisconnect = () => {
            this.displayText.visible = true;
            this.displayText.text = 'Disconnected';
        };
        this.network.onlaunch = async (seed) => {
        
            // hide display text
            this.displayText.visible = false;
            this.displayText.text = 'X';

            this.game.setup({
                seed: seed,
                playerCount: this.network.config.playerCount,
                local: this.network.local
            });

            console.log("sending ready frame");
            this.network.ready(); 

            // if players is more than one, download roster

            // get the update loop started!
            requestAnimationFrame((deltaTotal) => {
                this.lastTick = deltaTotal;
                this.update(deltaTotal);
            });

        };

        this.paused = false;
    
        return this;
    }

    resize() {
        // center text
        //this.displayText.x = window.innerWidth / 2.0;
        //this.displayText.y = window.innerHeight / 2.0;

        // scale invader screen
        const scaleX = window.innerWidth / this.container.virtualWidth;
        const scaleY = window.innerHeight / this.container.virtualHeight;
        const finalScale = Math.min(scaleX, scaleY);
        //const finalScale = 0.5;
        this.container.scale.set(finalScale);
        
        // Center the stage if needed
        //this.container.x = (window.innerWidth - (this.container.virtualWidth * finalScale)) / 2;
        //this.container.y = (window.innerHeight - (this.container.virtualHeight * finalScale)) / 2;
        this.container.x = window.innerWidth / 2.0;
        this.container.y = window.innerHeight / 2.0
    }
    
    update(deltaTotal) {
        let delta = deltaTotal - this.lastTick;
        this.lastTick = deltaTotal;

        if (this.paused == false) {
            // update input with delta
            // if there are any touches on left or right side of screen
            for (let k in this.pointerList) {							
                if (this.pointerList[k].x <= window.innerWidth / 2) { this.keys.left = true; }
                if (this.pointerList[k].x > window.innerWidth / 2)  { this.keys.right = true; }
            }
            if (this.keyLeft) { this.keys.left = true; }
            if (this.keyRight) { this.keys.right = true; }
            this.keys.primary = true;

            // true if input got sent over the network
            this.game.getInput(this.keys, this.network.inputBuffer);
            if (this.network.update(delta)) {
                // reset the input
                this.keys.left = false;
                this.keys.right = false;
            }
            while (this.network.needsUpdate()) {
                this.game.setInput(this.network.frameBuffer);
                this.game.update();							
            }        
            if (this.network.needsCopy()) {							
                this.game.copy();
            }
            while (this.network.needsFastForward()) {
                this.game.setForwardInput(this.network.frameBuffer);
                this.game.forward();							
            }
        }

        // this code renders the screen by reusing sprites like a "sprite batch"
        if (this.texture) {
            this.renderObjectIndex = 0;
            this.game.render(this);
            for (let i = this.renderObjectIndex; i < this.container.children.length; ++i) {
                this.container.children[i].visible = false;
            }

            // debug touch
            this.debugTouchIndex = 0;
            for (let k in this.pointerList) {
                if (this.debugTouch.children.length <= this.debugTouchIndex) {
                    this.debugTouch.addChild(new PIXI.Sprite());
                }
                let sprite = this.debugTouch.children[this.debugTouchIndex++];
                sprite.anchor.set(0.5);
                sprite.visible = true;
                sprite.texture = this.texture.textures[62];
                sprite.x = this.pointerList[k].x;
                sprite.y = this.pointerList[k].y;
                sprite.scale.set(64.0);
            }
            for (let i = this.debugTouchIndex; i < this.debugTouch.children.length; ++i) {
                this.debugTouch.children[i].visible = false;
            }
        }

        this.displayText.visible = this.network.stalled;

        if (this.visualizer.visible) {
            this.visualizer.update(this.network);
        }

        // only request if we are still connected!
        if (this.network.connected) {
            requestAnimationFrame(this.update.bind(this));
        }
    }

    // x, y, frame, scale, color
    sprite(x, y, frame, scale, alpha) {
        if (this.container.children.length <= this.renderObjectIndex) {
            this.container.addChild(new PIXI.Sprite());
        }
        let sprite = this.container.children[this.renderObjectIndex++];
        sprite.anchor.set(0.5);
        sprite.visible = true;
        sprite.texture = this.texture.textures[frame];
        sprite.x = x;
        sprite.y = y;
        sprite.alpha = alpha;
        sprite.scale.set(scale);
        //return sprite;
    }

}