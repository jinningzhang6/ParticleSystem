#version 330 compatibility


uniform float uA,uB,uC,uD;
varying vec3 vRefractVector;
varying vec3 vReflectVector;
uniform float uEta;


varying vec3  vnormal;
varying vec3 vMC;

const float pi=3.14159326;

void
main( )
{
	vMC=  gl_Vertex.xyz;
	float x=gl_Vertex.x;
	float y=gl_Vertex.y;
	float r= sqrt(x*x + y*y);
	float z= uA * cos(2*pi*uB*r + uC)* exp(-uD*r);

	vec4 new= vec4(x,y,z,1);
	float drdx=x/r;
	float drdy=y/r;
	float neg = sin(2*pi*uB*r + uC);
	float dzdr= uA * ((-neg) * 2.*pi*uB * exp(-uD*r) + cos(2.*pi*uB*r+uC) * -uD * exp(-uD*r) );

	float dzdx = dzdr*drdx;
	float dzdy = dzdr*drdy;
	vec3 Tx = vec3(1., 0., dzdx );
	vec3 Ty = vec3(0., 1., dzdy );
	vnormal=normalize(cross(Tx,Ty));

  vec3 ECposition = vec3( gl_ModelViewMatrix * gl_Vertex );
  vec3 eyeDir = normalize( ECposition); // vector from eye to pt
  //vec3 normal = normalize( gl_NormalMatrix * gl_Normal );
  vRefractVector = refract( eyeDir, vnormal, uEta );
  vReflectVector = reflect( eyeDir, vnormal );
	gl_Position = gl_ModelViewProjectionMatrix * new;
}
