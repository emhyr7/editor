#version 330

layout(location = 0) in vec3 input_position;

void main()
{
	gl_Position = vec4(input_position, 1.0);
}
