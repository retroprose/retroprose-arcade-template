// main game class
class Main {

    async init(config) {
        // initialize pixi.js
        this.pixi = new PIXI.Application();
        await this.pixi.init({ background: '#000000', resizeTo: window });
        document.body.appendChild(this.pixi.canvas);
        PIXI.TextureStyle.defaultOptions.scaleMode = 'nearest';					

        this.game = new Module.BindGame();
        this.bindInput = new Module.BindInput();

        this.texture = await PIXI.Assets.load('./images/sprite.json');

        const tm = await fetch('./data/tilemap.json');
        this.tilemap = await tm.json();

        const ob = await fetch('./data/objects.json');
        this.docks = await ob.json();

    
        this.renderObjectIndex = 0;

        this.config = config;

        // container for game objects
        //this.container = new PIXI.Container();
        //this.container.virtualWidth = 720;
        //this.container.virtualHeight = 405;
        //this.pixi.stage.addChild(this.container);

        // world and camera coordinates
        this.world = new PIXI.Container();
        this.space = new PIXI.Container();
        this.grid = new PIXI.Container();
        this.space2 = new PIXI.Container();
        this.camera = new PIXI.Container();
        this.screen = new PIXI.Container();

        this.screen.virtualWidth = 720;
        this.screen.virtualHeight = 405;
        this.camera.x = this.screen.virtualWidth / 2;
        this.camera.y = this.screen.virtualHeight / 2 + this.screen.virtualHeight / 4;

        this.camera.addChild(this.space2);
        this.camera.addChild(this.space);
        this.camera.addChild(this.grid);
        this.camera.addChild(this.world);
        this.screen.addChild(this.camera);
        this.pixi.stage.addChild(this.screen);

        /*let numStars = 200;
        let backgroundSize = 720;
        for (let i = 0; i < numStars*4; ++i) {
            let s = new PIXI.Sprite();            
            s.texture = this.texture.textures[0xdb];
            s.tint = 0xc8c8c8;
            s.width = 1.0;
            s.height = 1.0;
            this.space.addChild(s);
        }
        for (let i = 0; i < numStars; ++i) {
            let x = Math.random() * backgroundSize;
            let y = Math.random() * backgroundSize;
            this.space.getChildAt(i+0).x = x;
            this.space.getChildAt(i+0).y = y;
            this.space.getChildAt(i+1).x = x + backgroundSize;
            this.space.getChildAt(i+1).y = y;
            this.space.getChildAt(i+2).x = x;
            this.space.getChildAt(i+2).y = y + backgroundSize;
            this.space.getChildAt(i+3).x = x + backgroundSize;
            this.space.getChildAt(i+3).y = y + backgroundSize;
        }*/

        for (let i = 0; i < this.tilemap.length; ++i) {
            let t = this.tilemap[i];
            if (t > 0) {
                let s = new PIXI.Sprite();
                s.anchor.set(0.5); 
                if (t >= 97 && t <= 122) {
                    s.texture = this.texture.textures[0x0f];
                    let c = (i % 300);
                    let r = Math.floor(i / 300);
                    if (c <= 58)         { s.tint = 0xb21030 }
                    else if (r <= 59)    { s.tint = 0x49a269 }
                    else if (c >= 241)   { s.tint = 0xffa200 }
                    else if (r >= 241)   { s.tint = 0xa271ff }
                } else {
                    s.texture = this.texture.textures[t];
                    s.tint = 0xa9a9a9;                
                }
                s.x = (i % 300) * 8 + 4;
                s.y = Math.floor(i / 300) * 8 + 4;
                this.grid.addChild(s);
            }
        }

        // make stars
        let numStars = 500;
        let backgroundSize = 300 * 8;
        for (let i = 0; i < numStars; ++i) {
            let s = new PIXI.Sprite();            
            s.texture = this.texture.textures[0xdb];
            s.tint = 0x888888;
            s.width = 1.0;
            s.height = 1.0;
            s.x = Math.random() * backgroundSize;
            s.y = Math.random() * backgroundSize;
            this.space.addChild(s);
        }

        for (let i = 0; i < numStars; ++i) {
            let s = new PIXI.Sprite();            
            s.texture = this.texture.textures[0xdb];
            s.tint = 0x383838;
            s.width = 1.0;
            s.height = 1.0;
            s.x = Math.random() * backgroundSize;
            s.y = Math.random() * backgroundSize;
            this.space2.addChild(s);
        }

        this.cameraData = {
            x: 0.0,
            y: 0.0,
            a: 0.0
        }

        this.visualizer = new NetworkVisualizer();
        this.visualizer.visible = false;
        this.pixi.stage.addChild(this.visualizer);

        // system display text
        this.displayText = new PIXI.Text({
            text: 'Loading...',
            style: {
                fill: '#ffffff',
                fontSize: 24
            },
            anchor: 0.5
        });
        this.pixi.stage.addChild(this.displayText);

        // stall text
        this.stallText = new PIXI.Text({
            text: 'X',
            style: {
                fill: '#ffffff',
                fontSize: 24
            }
        });
        this.stallText.visible = false;
        this.pixi.stage.addChild(this.stallText);

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

        this.keyLeft = false;
        this.keyRight = false;
        this.keyPrimary = false
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
                this.keyPrimary = true;
            } else if (event.key == 'p') {
                this.paused = !this.paused;
            } else if (event.key == 'o') {
                this.advance = true;
            } else if (event.key == '\\') {
                this.visualizer.visible = !this.visualizer.visible;
            }
        });
        window.addEventListener('keyup', (event) => {
            if (event.key == 'ArrowLeft') {
                this.keyLeft = false;
            } else if (event.key == 'ArrowRight') {
                this.keyRight = false;
            } else if (event.key == ' ') {
                this.keyPrimary = false;
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
            this.displayText.text = '';

            // set up game here
            this.game.setup({
                seed: seed,
                playerCount: this.network.config.playerCount,
                local: this.network.local,
                map: this.tilemap,
                docks: this.docks
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
        this.advance = false;
    
        return this;
    }

    resize() {
        // center text
        this.displayText.x = window.innerWidth / 2.0;
        this.displayText.y = window.innerHeight / 2.0;

        // scale invader screen
        const scaleX = window.innerWidth / this.screen.virtualWidth;
        const scaleY = window.innerHeight / this.screen.virtualHeight;
        //const finalScale = Math.min(scaleX, scaleY);
        //const finalScale = 2.0;
        const finalScale = 1.0;
        this.screen.scale.set(finalScale);
        
        this.screen.x = (window.innerWidth - (this.screen.virtualWidth * finalScale)) / 2;
        this.screen.y = (window.innerHeight - (this.screen.virtualHeight * finalScale)) / 2;
    }
    
    update(deltaTotal) {
        let delta = deltaTotal - this.lastTick;
        this.lastTick = deltaTotal;

        if (this.paused == false || this.advance == true) {
            this.advance = false;
            // update input with delta
            // if there are any touches on left or right side of screen
            for (let k in this.pointerList) {	
                //if (this.pointerList[k].y < window.innerHeight / 2) {
                //    this.bindInput.primary = true;
                //} else {
                    if (this.pointerList[k].x < window.innerWidth / 2) {
                        this.bindInput.left = true;
                    } else if (this.pointerList[k].x >= window.innerWidth / 2) {
                        this.bindInput.right = true;
                    }
                //}
            }
            if (this.keyLeft) { this.bindInput.left = true; }
            if (this.keyRight) { this.bindInput.right = true; }
            if (this.keyPrimary) { this.bindInput.primary = true; }

            // true if input got sent over the network
            this.game.getInput(this.bindInput, this.network.inputBuffer);
            if (this.network.update(delta)) {
                // reset the input
                this.bindInput.left = false;
                this.bindInput.right = false;
                this.bindInput.primary = false;
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
            for (let i = this.renderObjectIndex; i < this.world.children.length; ++i) {
                this.world.children[i].visible = false;
            }
        }

        this.world.x = this.cameraData.x;
        this.world.y = this.cameraData.y;
        this.grid.x = this.cameraData.x;
        this.grid.y = this.cameraData.y;
        
        this.space.x = this.cameraData.x / 2.0;  // have this wrap around
        this.space.y = this.cameraData.y / 2.0;
        this.space2.x = this.cameraData.x / 4.0;  // have this wrap around
        this.space2.y = this.cameraData.y / 4.0;
        
        this.camera.angle = this.cameraData.a;
        //this.camera.angle = this.lerpAngle(this.camera.angle, this.cameraData.a, (delta / 1000.0) * 2.5);

        this.stallText.visible = this.network.stalled;

        if (this.visualizer.visible) {
            this.visualizer.update(this.network);
        }

        // only request if we are still connected!
        if (this.network.connected) {
            requestAnimationFrame(this.update.bind(this));
        }
    }

    lerpAngle(p_from, p_to, p_weight) {
        let difference = (p_to - p_from) % 360;
        let distance = ((2.0 * difference) % 360) - difference;
        return p_from + distance * p_weight;    
    }

    setCamera(x, y, angle) {
        this.cameraData.x = -x;
        this.cameraData.y = -y;
        this.cameraData.a = -angle;
    }

    setTeam(team) {
        //console.log(team);
        this.displayText.visible = true;
        switch (team) {
            case 0: this.displayText.text = 'Red Team'; break;
            case 1: this.displayText.text = 'Green Team'; break;
            case 2: this.displayText.text = 'Yellow Team'; break;
            case 3: this.displayText.text = 'Blue Team'; break;
            case 11: this.displayText.text = 'Get Ready'; break;
            default: this.displayText.text = ''; this.displayText.visible = false; break;
        }
    }

    // x, y, frame, scale, color
    sprite(x, y, frame, color, angle=0.0, alpha=1.0, scale=1.0) {
        if (this.world.children.length <= this.renderObjectIndex) {
            this.world.addChild(new PIXI.Sprite());
        }
        let sprite = this.world.children[this.renderObjectIndex++];
        sprite.anchor.set(0.5);
        sprite.visible = true;
        sprite.texture = this.texture.textures[frame];
        sprite.x = x;
        sprite.y = y;
        sprite.angle = angle;
        sprite.tint = color;
        sprite.alpha = alpha;
        sprite.scale.set(scale);
    }

}