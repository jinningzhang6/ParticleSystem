#version 330 compatibility

uniform float uNoiseAmp;
uniform float uNoiseFreq;
uniform sampler3D Noise3;
varying vec3 vMC;

varying vec3 vReflectVector;
varying vec3 vRefractVector;
uniform float uMix;
uniform samplerCube uReflectUnit;
uniform samplerCube uRefractUnit;
const vec4 WHITE = vec4( 1.,1.,1.,1. );
varying vec3 vnormal;

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

void main(){
  vec4 nvx = texture( Noise3, uNoiseFreq*vMC );
	float angx = nvx.r + nvx.g + nvx.b + nvx.a  -  2.;
	angx *= uNoiseAmp;

  vec4 nvy = texture( Noise3, uNoiseFreq*vec3(vMC.xy,vMC.z+0.5) );
	float angy = nvy.r + nvy.g + nvy.b + nvy.a  -  2.;
	angy *= uNoiseAmp;

  vec3 anotheRefract;
  vec3 anotheReflect;
	anotheRefract = RotateNormal(angx,angy,vRefractVector);
  anotheReflect = RotateNormal(angx,angy,vReflectVector);
  vec4 refractcolor = textureCube( uRefractUnit, anotheRefract );
  vec4 reflectcolor = textureCube( uReflectUnit, anotheReflect );
  refractcolor = mix( refractcolor, WHITE, .40 );
  gl_FragColor = vec4( mix( refractcolor, reflectcolor, uMix ).rgb, 1. );
}
