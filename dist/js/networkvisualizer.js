// class used to visualize the network traffic
class NetworkVisualizer extends PIXI.Container {
    constructor() {
        super();
        // network frame sprite
        this.networkFrame = new PIXI.Graphics();
        this.networkFrame.beginFill(0xff0000);
        this.networkFrame.drawCircle(0, 0, 12);
        this.networkFrame.endFill();
        this.addChild(this.networkFrame);

        // network frame sprite
        this.updateFrame = new PIXI.Graphics();
        this.updateFrame.x = 24 * 1;
        this.updateFrame.beginFill(0x0000ff);
        this.updateFrame.drawCircle(0, 0, 12);
        this.updateFrame.endFill();
        this.addChild(this.updateFrame);

        // network frame sprite
        this.fastForwardFrame = new PIXI.Graphics();
        this.fastForwardFrame.x = 24 * 2;
        this.fastForwardFrame.beginFill(0xc8c8c8);
        this.fastForwardFrame.drawCircle(0, 0, 12);
        this.fastForwardFrame.endFill();
        this.addChild(this.fastForwardFrame);

        // network frame sprite
        this.sentFrame = new PIXI.Graphics();
        this.sentFrame.x = 24 * 3;
        this.sentFrame.beginFill(0xffff00);
        this.sentFrame.drawCircle(0, 0, 12);
        this.sentFrame.endFill();
        this.addChild(this.sentFrame);

        this.frameDisplay = [];
        for (let i = 0; i < 50; ++i) {
            const text = new PIXI.Text({
                text: '0',
                style: {
                    fill: '#ffffff',
                    fontSize: 24
                },
                anchor: 0.5
            });
            this.addChild(text);
            this.frameDisplay.push(text);
        }
    }

    destroy() {
        super.destroy();
    }

    update(network) {
        this.fastForwardFrame.y = network.fastForwardFrame * 24;
        this.sentFrame.y = network.bufferedFrame * 24;
        this.updateFrame.y = network.updateFrame * 24;

        this.networkFrame.y = network.networkFrame * 24;
        for (let i = 0; i < this.frameDisplay.length; ++i) {
            const frame = network.networkFrame - 25 + i;
            this.frameDisplay[i].text = "" + frame;
            this.frameDisplay[i].x = -128.0;
            this.frameDisplay[i].y = frame * 24;
        }

        this.x = window.innerWidth / 2.0;
        this.y = -this.sentFrame.y + window.innerHeight / 2.0;
    }
}