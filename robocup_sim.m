function robocup_sim()
    figure('Name', 'RoboCup SSL Simulation', 'NumberTitle', 'off', 'Color', 'w');

    % Field dimensions
    fieldLength = 9;
    fieldWidth = 6;

    % Simulation parameters
    numFrames = 200;
    dt = 0.05;  % time between frames (seconds)

    for t = 1:numFrames
        clf;
        hold on;
        axis equal;
        xlim([-fieldLength/2, fieldLength/2]);
        ylim([-fieldWidth/2, fieldWidth/2]);

        % Draw green field
        rectangle('Position', [-fieldLength/2, -fieldWidth/2, fieldLength, fieldWidth], ...
                  'FaceColor', [0.1, 0.6, 0.1], 'EdgeColor', 'k', 'LineWidth', 2);

        % Center line and circle
        line([0, 0], [-fieldWidth/2, fieldWidth/2], 'Color', 'w', 'LineStyle', '--', 'LineWidth', 1.5);
        viscircles([0, 0], 0.5, 'Color', 'w', 'LineStyle', ':');

        % === Blue Team Movements (circular motion) ===
        theta = 2 * pi * t / numFrames;
        blueTeam = [ cos(theta)*2, sin(theta)*1.5;
                     cos(theta + pi/2)*1.5, sin(theta + pi/2)*1.2;
                     cos(theta + pi)*1.2, sin(theta + pi)*1 ];

        % === Yellow Team Movements (linear back and forth) ===
        x_movement = 2 * sin(theta);  % oscillating x
        yellowTeam = [ x_movement,  1;
                      -x_movement, -1;
                       x_movement/2, 0 ];

        % Draw both teams
        for i = 1:size(blueTeam,1)
            draw_robot(blueTeam(i,:), 'blue');
        end
        for i = 1:size(yellowTeam,1)
            draw_robot(yellowTeam(i,:), 'yellow');
        end

        drawnow;
        pause(dt);
    end
end

function draw_robot(pos, color)
    r = 0.09; % Robot radius
    rectangle('Position', [pos(1)-r, pos(2)-r, 2*r, 2*r], ...
              'Curvature', [1 1], ...
              'FaceColor', color, ...
              'EdgeColor', 'k');

    % Heading (optional, points right for now)
    headingLength = 0.12;
    angle = 0;  % fixed heading
    hx = pos(1) + headingLength * cos(angle);
    hy = pos(2) + headingLength * sin(angle);
    line([pos(1), hx], [pos(2), hy], 'Color', 'k', 'LineWidth', 1.5);
end