#version 330 compatibility

varying vec3  vMCposition;
varying float vLightIntensity;
varying vec2 vST;
varying vec3 vColor;
varying vec4 vECposition;
varying float uA,uB,uC,uD;

varying vec3  vnormal;
varying vec3 vMC;
const float pi=3.14159326;
varying vec3 Lf;
varying vec3 Ls;
vec3 eyeLightPosition = vec3( 0, 0, 0 );
vec3 LIGHTPOS   = vec3( -2., 0., 10. );

void
main( )
{
	vST = gl_MultiTexCoord0.st;

	vMC=  gl_Vertex.xyz;
	float x=gl_Vertex.x;
	float y=gl_Vertex.y;
	float r= sqrt(x*x + y*y);
	float z= uA * cos(2*pi*uB*r + uC)* exp(-uD*r);


	float drdx=x/r;
	float drdy=y/r;
	float neg = sin(2*pi*uB*r + uC);
	float dzdr= uA * ((-neg) * 2.*pi*uB * exp(-uD*r) + cos(2.*pi*uB*r+uC) * -uD * exp(-uD*r) );

	float dzdx = dzdr*drdx;
	float dzdy = dzdr*drdy;
	vec3 Tx = vec3(1., 0., dzdx );
	vec3 Ty = vec3(0., 1., dzdy );
	vnormal=normalize(cross(Tx,Ty));

	vec3 tnorm      = normalize( gl_NormalMatrix * gl_Normal );
	vECposition = gl_ModelViewMatrix * gl_Vertex;
	vec3 ECposition = vec3( gl_ModelViewMatrix * gl_Vertex );
	vLightIntensity = abs( dot( normalize(LIGHTPOS - ECposition), tnorm ) );
	vMCposition  = gl_Vertex.xyz;
	vColor = gl_Color.rgb;

	vec4 ECposition2 = gl_ModelViewMatrix * gl_Vertex;
	Lf = eyeLightPosition - ECposition2.xyz;		// vector from the point									
	Ls = Lf;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
