$fa = 1;
$fs = 0.5;

part = "LED holder"; // [LED holder, Cover]

/* [LED holder] */
led_diameter_1 = 5.5;
led_diameter_2 = 5.5;
led_holder_height = 8;

// Shouldn't need to be modified
led_spacing = 6.5;
// Shouldn't need to be modified
led_holder_diameter = 32;

/* [Cover] */
cover_wall_thickness = 1;
cover_clearance = 0.3;
cover_height = 14;
// How much of the LED holder goes into the cover
cover_led_holder_depth = 5;
cover_raised_ring_width = 2;
cover_cable_hole_diameter = 4;

module led_holder()
{
	difference() {
		cylinder(d=led_holder_diameter, h=led_holder_height);

		translate([-led_spacing/2, 0, -0.1]) //-0.1 on Z to make the preview look correct
			cylinder(d=led_diameter_1, h=led_holder_height + 0.2);
		translate([led_spacing/2, 0, -0.1])
			cylinder(d=led_diameter_2, h=led_holder_height + 0.2);
	}
}

module cover()
{
	inner_diameter = led_holder_diameter + cover_clearance;
	difference() {
		union() {
			difference() {
				cylinder(d=inner_diameter + cover_wall_thickness * 2, h=cover_height + cover_wall_thickness);

				translate([0, 0, cover_wall_thickness])
					cylinder(d=inner_diameter, h=cover_height + 0.1);
			}

			// The raised ring that stops the LED holder from going in too far
			h = cover_height - cover_led_holder_depth;
			translate([0, 0, cover_wall_thickness])
				difference() {
					cylinder(d=inner_diameter, h=h);

					cylinder(d=inner_diameter - cover_raised_ring_width * 2, h=h + 0.1);
				}
		}

		translate([0, 0, cover_wall_thickness + cover_cable_hole_diameter / 2]) rotate([90, 0, 0])
			cylinder(d=cover_cable_hole_diameter, h=inner_diameter);
	}
}

if(part == "LED holder")
	led_holder();
else if(part == "Cover")
	cover();