#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Rand.h"
#include "ParticleSystem.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Text.h"
#include "cinder/Perlin.h"
#include "cinder/Camera.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Follower{
public:
    Follower(){
        theta = 0.0f;
        phi = 0.0f;
        radius = Rand::randFloat(0.5f, 10.0f);
    }
    
    void moveTo( const Vec3f& target ){
        prevPos = pos;
        pos += ( target - pos ) * 0.1f;
    }
    
    void draw(){
        //gl::drawSphere( pos, radius, 4);
        ci::gl::drawSolidRect(ci::Rectf(pos.x-radius, pos.y-radius, pos.x+radius, pos.y+radius));
        //Vec3f vel = pos - prevPos;
        //gl::drawLine( pos, pos + ( vel * 20.0f ) );
    }
    
    Vec3f pos, prevPos;
    float phi, theta, radius;
};

class ParticleNoiseApp : public AppNative {
public:
    void prepareSettings(Settings *settings);
	void setup();
	void mouseMove(MouseEvent event);
	void mouseDown( MouseEvent event );
    void keyDown(KeyEvent event);
	void update();
	void draw();
    
    void touchBegan(TouchEvent event);
    void touchMoved(TouchEvent event);
    
private:
    int windowWidth;
    int windowHeight;
    
    Font mFont;
    string fpsStr;
    
    ParticleSystem mParticleSystem;
    Vec2f   attrPosition;
    float   attrFactor, repulsionFactor;
    
    bool    mRunning;
    
    float   attrRadius;
    gl::Texture particleTexture;
    CameraPersp mCam;
    
    //perlin
    Vec3f sphericalToCartesians(float radius, float theta, float phi);
    vector<shared_ptr< Follower >> mFollowers;
    vector<shared_ptr< Follower >> mFollowers2;
    float mRadius;
    float mCounter;
    Perlin mPerlin;
    gl::Texture followerTexture;
    gl::Texture followerTexture2;
};
void ParticleNoiseApp::prepareSettings(Settings *settings)
{
    windowWidth = 640;
    windowHeight = 640;
    settings->enableMultiTouch();
    settings->setWindowSize(windowWidth, windowHeight);
    settings->setFrameRate(60.0f);
    //settings->setFullScreen( true );
    
}

void ParticleNoiseApp::setup()
{
    mRunning = true;
    attrPosition = getWindowCenter();
    attrRadius = 100.f;
    attrFactor = 0.01f;
    repulsionFactor = -5.f;
    int numParticle = 2000;
    for( int i=0; i<numParticle; i++ ){
        float x = Rand::randFloat( 0.0f, getWindowWidth() );
        float y = Rand::randFloat( 0.0f, getWindowHeight() );
        float radius = Rand::randFloat(0.5f, 8.0f);
        float mass = radius*2.f;
        float drag = 0.95f;
        Particle *particle = new Particle( Vec2f( x, y ), radius, mass, drag );
        mParticleSystem.addParticle( particle );
    }
    
    particleTexture = gl::Texture(loadImage(loadAsset("reflection.png")));
    
    
    //mFont = Font(loadResource("A-OTF-ShinGoPro-Light.otf"),16.0f);
    
    //perlin
    mRadius = 100.0f;
    mCounter = 0.0f;
    
    int numFollowers = 800;
    int numFollowers2 = 1000;
    for(int i = 0; i < numFollowers; i++){
        shared_ptr<Follower> follower(new Follower());
        follower->theta = randFloat( M_PI * 2.0f );
        follower->phi = randFloat( M_PI * 2.0f );
        follower->pos = sphericalToCartesians( mRadius, follower->theta, follower->phi );
        mFollowers.push_back( follower );
    }
    for(int j = 0; j < numFollowers2; j++){
        shared_ptr<Follower> follower(new Follower());
        follower->theta = randFloat( M_PI * 2.0f );
        follower->phi = randFloat( M_PI * 2.0f );
        follower->pos = sphericalToCartesians( mRadius, follower->theta, follower->phi );
        mFollowers2.push_back( follower );
    }
    followerTexture = gl::Texture(loadImage(loadAsset("particle2.png")));
    followerTexture2 = gl::Texture(loadImage(loadAsset("particle4.png")));
}
void ParticleNoiseApp::touchMoved(TouchEvent event){
    for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt ){
     
        attrPosition = touchIt->getPos();
        
    }
        
}
void ParticleNoiseApp::touchBegan(TouchEvent event){
    
    Vec2f p;
    for( vector<TouchEvent::Touch>::const_iterator touchIt = event.getTouches().begin(); touchIt != event.getTouches().end(); ++touchIt ){
         p = touchIt->getPos();
    }
   
    for( std::vector<Particle*>::iterator it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it ) {
        Vec2f repulsionForce = (*it)->position - p;
        repulsionForce = repulsionForce.normalized() * math<float>::max(0.f, 100.f - repulsionForce.length());
        (*it)->forces += repulsionForce;
    }
}

void ParticleNoiseApp::mouseMove( MouseEvent event )
{
    attrPosition.x = event.getPos().x;
    attrPosition.y = event.getPos().y;
}
void ParticleNoiseApp::mouseDown( MouseEvent event )
{
    for( std::vector<Particle*>::iterator it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it ) {
        Vec2f repulsionForce = (*it)->position - event.getPos();
        repulsionForce = repulsionForce.normalized() * math<float>::max(0.f, 100.f - repulsionForce.length());
        (*it)->forces += repulsionForce;
    }
}
void ParticleNoiseApp::keyDown(KeyEvent event){
    if(event.getChar() == ' ') {
        mRunning = !mRunning;
    }
}
void ParticleNoiseApp::update()
{
    if(!mRunning)
        return;
    
    for( std::vector<Particle*>::iterator it = mParticleSystem.particles.begin(); it != mParticleSystem.particles.end(); ++it ) {
        Vec2f attrForce = attrPosition - (*it)->position;
        if( attrForce.length() <= attrRadius+(*it)->radius*0.5f )
            attrForce = attrForce.normalized() * -2000.f;
        
        attrForce *= attrFactor;
        (*it)->forces += attrForce;
    }
    
    mParticleSystem.update();
    
    
    //perlin
    mCounter += 0.01f;
    
    float resolution = 0.01f;
    for(int i = 0; i < mFollowers.size(); i++){
        shared_ptr<Follower> follower = mFollowers[i];
        Vec3f pos = follower->pos;
        float thetaAdd = mPerlin.noise( pos.x * resolution, pos.y * resolution, mCounter ) * 0.1f;
        float phiAdd = mPerlin.noise( pos.y * resolution, pos.z * resolution, mCounter ) * 0.1f;
        follower->theta += thetaAdd;
        follower->phi += phiAdd;
        Vec3f targetPos = sphericalToCartesians( mRadius, follower->theta, follower->phi );
        follower->moveTo( targetPos );
    }
    for(int j = 0; j < mFollowers2.size(); j++){
        shared_ptr<Follower> follower = mFollowers2[j];
        Vec3f pos = follower->pos;
        float thetaAdd = mPerlin.noise( pos.x * resolution, pos.y * resolution, mCounter ) * 0.12f;
        float phiAdd = mPerlin.noise( pos.y * resolution, pos.z * resolution, mCounter ) * 0.08f;
        follower->theta += thetaAdd;
        follower->phi += phiAdd;
        Vec3f targetPos = sphericalToCartesians( mRadius, follower->theta, follower->phi );
        follower->moveTo( targetPos );
    }

}

void ParticleNoiseApp::draw()
{
	gl::enableAlphaBlending();
    gl::enableAdditiveBlending();
	gl::clear( Color::black() );
    gl::setMatricesWindow(getWindowSize());
	 
    //int particleNum = mParticleSystem.particles.size();
    
    particleTexture.enableAndBind();
    mParticleSystem.draw();
    particleTexture.unbind();
    gl::color(ColorA(0.3f,0.3f,0.3f,0.8f));
    gl::drawStrokedCircle(attrPosition, attrRadius);
    
    
    // clear out the window with black
	gl::setMatricesWindowPersp(getWindowWidth(), getWindowHeight());
    
    gl::pushMatrices();
    //Vec2f center = getWindowCenter();
    Vec2f center = attrPosition;
    gl::translate( center );
   
    gl::color( Color( 1.0f, 1.0f, 1.0f ));
    followerTexture.enableAndBind();
    for(vector<shared_ptr<Follower>>::iterator it = mFollowers.begin(); it != mFollowers.end(); ++it){
        (*it)->draw();
    }
    followerTexture.unbind();
    
    gl::color( Color( 1.0f, 1.0f, 1.0f ));
    followerTexture2.enableAndBind();
    for(vector<shared_ptr<Follower>>::iterator it = mFollowers2.begin(); it != mFollowers2.end(); ++it){
        (*it)->draw();
    }
    followerTexture2.unbind();
    
    gl::popMatrices();
    /*
    //FPS表示
    fpsStr = toString(getAverageFps());
    gl::drawString( "FPS = " + fpsStr
                   ,Vec2f(20.0f,20.0f)
                   ,Color::white()
                   ,mFont);
    */
   
}
Vec3f ParticleNoiseApp::sphericalToCartesians(float radius, float theta, float phi){
    float x = radius * sinf( theta ) * cosf( phi );
    float y = radius * sinf( theta ) * sinf( phi );
    float z = radius * cosf( theta );
    
    return Vec3f( x, y, z );
};

CINDER_APP_NATIVE( ParticleNoiseApp, RendererGl )
