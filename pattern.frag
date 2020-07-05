#version 330 compatibility

varying vec2 vST;
varying vec3 vMCposition;
varying float vLightIntensity;
varying vec3 vColor;
varying vec4 uColor;

uniform float uAd;
uniform float uBd;
uniform float uNoiseAmp;
uniform float uNoiseFreq;
uniform float uTol;
uniform float uAlpha;
uniform bool  uUseChromaDepth;
uniform float uChromaBlue;
uniform float uChromaRed;
varying vec3 ECposition;
varying vec4 vECposition;

varying vec3 vMC;
varying vec3 vnormal;

const vec3 RED = vec3( 1., 1., 0. );
const vec3 BLUE= vec3(1.,0.,0.);

uniform sampler3D Noise3;

vec3
Rainbow( float d )
{
	d = clamp( d, 0., 1. );

	float r = 1.;
	float g = 0.0;
	float b = 1.  -  6. * ( d - (5./6.) );

        if( d <= (5./6.) )
        {
                r = 6. * ( d - (4./6.) );
                g = 0.;
                b = 1.;
        }

        if( d <= (4./6.) )
        {
                r = 0.;
                g = 1.  -  6. * ( d - (3./6.) );
                b = 1.;
        }

        if( d <= (3./6.) )
        {
                r = 0.;
                g = 1.;
                b = 6. * ( d - (2./6.) );
        }

        if( d <= (2./6.) )
        {
                r = 1.  -  6. * ( d - (1./6.) );
                g = 1.;
                b = 0.;
        }

        if( d <= (1./6.) )
        {
                r = 1.;
                g = 6. * d;
        }

	return vec3( r, g, b );
}

vec3
RotateNormal( float angx, float angy, vec3 n )
{
        float cx = cos( angx );
        float sx = sin( angx );
        float cy = cos( angy );
        float sy = sin( angy );

        // rotate about x:
        float yp =  n.y*cx - n.z*sx;    // y'
        n.z      =  n.y*sx + n.z*cx;    // z'
        n.y      =  yp;
        // n.x      =  n.x;

        // rotate about y:
        float xp =  n.x*cy + n.z*sy;    // x'
        n.z      = -n.x*sy + n.z*cy;    // z'
        n.x      =  xp;
        // n.y      =  n.y;

        return normalize( n );
}

void
main( )
{
//uColor=(1.,0.7,1);
vec3 stp = uNoiseFreq * vMCposition;
vec4 nv  = texture( Noise3, stp );
float n = nv.r + nv.g + nv.b + nv.a;    //  1. -> 3.
n = n - 2.;                             // -1. -> 1.

n= n*uNoiseAmp;
// determine the color based on the noise-modified (s,t):
float Ar= uAd/2;
float Br= uBd/2;
int numins= int(vST.s/uAd);
int numint= int(vST.t/uBd);

float sc = float(numins) * uAd  +  Ar;
float ds = vST.s - sc;                   // wrt ellipse center
float tc = float(numint) * uBd  +  Br;
float dt = vST.t - tc;                   // wrt ellipse center


vec3 Normal;
vec3 Light;
Normal = RotateNormal(0,0,vnormal);

float dmy = max( dot(Normal,Light), 0. );
vec4 diffuse = 0.6 * dmy * uColor;

float oldDist = sqrt( ds*ds + dt*dt );
float newDist = oldDist+n;
float scale = newDist / oldDist;        // this could be < 1., = 1., or > 1.

ds *= scale;                            // scale by noise factor
ds /= Ar;                               // ellipse equation

dt *= scale;                            // scale by noise factor
dt /= Br;                               // ellipse equation

float d = ds*ds + dt*dt;
float t = smoothstep(1 - uTol, 1 + uTol, d);
vec4 nBLUE=vec4(BLUE,uAlpha);
vec4 nRED=vec4(RED,1);

vec4 color=vLightIntensity* mix(nBLUE, nRED, t);
if(uAlpha==0.){
  discard;
}

if( uUseChromaDepth ){
		float t = (2./3.) * ( vECposition.z - uChromaRed ) / ( uChromaBlue - uChromaRed );
		t = clamp( t, 0., 2./3. );
		color = vec4(Rainbow( t ), uAlpha);
}


gl_FragColor= color+ vec4(diffuse.rgb,1);


}
