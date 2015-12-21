#include "framework.h"

#include "particle.h"

using namespace prototyper;

framework frm;


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
bool move_camera( camera<float>& cam, float move_amount, float seconds )
{
  if( sf::Keyboard::isKeyPressed( sf::Keyboard::LShift ) || sf::Keyboard::isKeyPressed( sf::Keyboard::RShift ) )
  {
    move_amount = move_amount * 3.0f;
  }
  else
  {
    move_amount = move_amount;
  }

  static vec3 movement_speed = vec3( 0 );

  //move camera
  if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
  {
    movement_speed.x -= move_amount;
  }

  if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
  {
    movement_speed.x += move_amount;
  }

  if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
  {
    movement_speed.y += move_amount;
  }

  if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
  {
    movement_speed.y -= move_amount;
  }

  if( length( movement_speed ) > 0.25 )
  {
    cam.move_forward( movement_speed.y * seconds );
    cam.move_right( movement_speed.x * seconds );
    movement_speed *= 0.5;

    return true;
  }

  return false;
}

int main( int argc, char** argv )
{
  map<string, string> args;

  for( int c = 1; c < argc; ++c )
  {
    args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
    ++c;
  }

  cout << "Arguments: " << endl;
  for_each( args.begin(), args.end(), []( pair<string, string> p )
  {
    cout << p.first << " " << p.second << endl;
  } );

  uvec2 screen( 0 );
  bool fullscreen = false;
  bool silent = false;
  string title = "Particle System";

  /*
   * Process program arguments
   */

  stringstream ss;
  ss.str( args["--screenx"] );
  ss >> screen.x;
  ss.clear();
  ss.str( args["--screeny"] );
  ss >> screen.y;
  ss.clear();

  if( screen.x == 0 )
  {
    screen.x = 1280;
  }

  if( screen.y == 0 )
  {
    screen.y = 720;
  }

  try
  {
    args.at( "--fullscreen" );
    fullscreen = true;
  }
  catch( ... )
  {
  }

  try
  {
    args.at( "--help" );
    cout << title << ", written by Marton Tamas." << endl <<
      "Usage: --silent      //don't display FPS info in the terminal" << endl <<
      "       --screenx num //set screen width (default:1280)" << endl <<
      "       --screeny num //set screen height (default:720)" << endl <<
      "       --fullscreen  //set fullscreen, windowed by default" << endl <<
      "       --help        //display this information" << endl;
    return 0;
  }
  catch( ... )
  {
  }

  try
  {
    args.at( "--silent" );
    silent = true;
  }
  catch( ... )
  {
  }

  /*
   * Initialize the OpenGL context
   */

  frm.init( screen, title, fullscreen );
  frm.set_vsync( true );


  //set opengl settings
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.5f, 0.5f, 0.8f, 0.0f ); //sky color
  glClearDepth( 1.0f );

  frm.get_opengl_error();

  /*
   * Set up mymath
   */

  camera<float> cam;
  frame<float> the_frame;

  cam.move_forward( -100 );

  float cam_fov = 45.0f;
  float cam_near = 1.0f;
  float cam_far = 1000.0f;

  the_frame.set_perspective( radians( cam_fov ), (float)screen.x / (float)screen.y, cam_near, cam_far );

  glViewport( 0, 0, screen.x, screen.y );

  /*
   * Set up the scene
   */

  GLuint tex = frm.load_image( "../resources/particles/soapbubble.png" );

  particle_manager pm;
  pm.init();

  //////////////////////////////////////////////////
  //first particle system
  //////////////////////////////////////////////////
  int ps_id = pm.create();
  auto ps = pm.get( ps_id );
  ps->pos = vec3( 0 );
  ps->dir = vec3( 1, 1, 0 );
  ps->duration = 5; //s
  ps->is_looping = true;
  ps->prewarm = false;
  ps->is_child = false;
  ps->is_additive = false;
  ps->is_stretched = true;
  ps->stretch_factor = 5;
  ps->inherit_vel = false;
  ps->gravity_multiplier = 5;
  ps->max_particles = 50000;
  ps->start_pos.type = FUNCTION;
  ps->start_pos.func = []( float dt, const vec3& pos, const vec3& dir ) -> vec3 
  {
    return pos + vec3( frm.get_random_num( 0, 1 ), 0, frm.get_random_num( 0, 1 ) ) * 0.5;
  };
  ps->start_velocity.type = FUNCTION;
  ps->start_velocity.func = []( float dt, const vec3& pos, const vec3& dir ) -> vec3
  {
    vec3 rand_vec = vec3( frm.get_random_num( -1, 1 ), frm.get_random_num( -1, 1 ), frm.get_random_num( -1, 1 ) );
    rand_vec *= sign( dot( rand_vec, vec3( 0, 1, 0 ) ) );
    return (dir + rand_vec * 0.25) * 30;
  };
  ps->start_color.type = CONSTANT;
  ps->start_color.value = vec3( 1 ); //rgb
  ps->start_size.type = CONSTANT;
  ps->start_size.value = 1;
  ps->start_opacity.type = CONSTANT;
  ps->start_opacity.value = 1; //[0..1]
  ps->emit_per_second.type = CONSTANT;
  ps->emit_per_second.value = 100; //pieces
  ps->start_life.type = CONSTANT;
  ps->start_life.value = 2; //s

  ps->bursts.push_back( make_pair( 0, 30 ) );
  ps->bursts.push_back( make_pair( 2.5, 30 ) );

  //////////////////////////////////////////////////
  //second particle system
  //////////////////////////////////////////////////
  int ps_id2 = pm.create();
  auto ps2 = pm.get( ps_id2 );
  ps2->duration = 0.5; //s
  ps2->is_looping = true;
  ps2->prewarm = false;
  ps2->is_child = true;
  ps2->is_additive = true;
  ps2->is_stretched = false;
  ps2->stretch_factor = 1;
  ps2->inherit_vel = true;
  ps2->gravity_multiplier = 5;
  ps2->max_particles = 50000;

  ps2->start_pos.type = FUNCTION;
  ps2->start_pos.func = []( float dt, const vec3& pos, const vec3& dir ) -> vec3
  {
    return pos + vec3( frm.get_random_num( 0, 1 ), 0, frm.get_random_num( 0, 1 ) ) * 0.5;
  };
  ps2->start_velocity.type = FUNCTION;
  ps2->start_velocity.func = []( float dt, const vec3& pos, const vec3& dir ) -> vec3
  {
    vec3 rand_vec = vec3( frm.get_random_num( -1, 1 ), frm.get_random_num( -1, 1 ), frm.get_random_num( -1, 1 ) );
    //rand_vec *= sign( dot( rand_vec, vec3( 0, 1, 0 ) ) );
    return ( rand_vec ) * 30;
  };
  ps2->start_color.type = CONSTANT;
  ps2->start_color.value = vec3( 1 ); //rgb
  ps2->start_size.type = CONSTANT;
  ps2->start_size.value = 1;
  ps2->start_opacity.type = CONSTANT;
  ps2->start_opacity.value = 1; //[0..1]
  ps2->emit_per_second.type = CONSTANT;
  ps2->emit_per_second.value = 0; //pieces
  ps2->start_life.type = CONSTANT;
  ps2->start_life.value = 1.5; //s

  ps2->bursts.push_back( make_pair( 0, 1 ) );

  //death emit
  ps->death_subemitter_ids.push_back( ps_id2 );

  float move_amount = 10;

  /*
   * Handle events
   */
  bool can_rotate = false;
  bool warped = false, ignore = true;
  vec2 mouse = vec2( 0 );

  float hori_sensitivity = 0.5;
  float vert_sensitivity = 0.15;

  bool has_focus = true;

  bool update_pm = true;

  auto event_handler = [&]( const sf::Event & ev )
  {
    switch( ev.type )
    {
    case sf::Event::GainedFocus:
    {
      has_focus = true;
      break;
    }
    case sf::Event::LostFocus:
    {
      has_focus = false;
      break;
    }
    case sf::Event::MouseMoved:
    {
      vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

      mouse = vec2( mpos.x, 1 - mpos.y );

      if( can_rotate )
      {
        if( warped )
        {
          ignore = false;
        }
        else
        {
          frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
          warped = true;
          ignore = true;
        }

        if( !ignore && all( notEqual( mpos, vec2( 0.5 ) ) ) )
        {
          //first rotate around global Y axis (horizontal)
          cam.rotate( radians( ( 0.5f - mpos.x ) * 2 * 180 * hori_sensitivity ), vec3( 0, 1, 0 ) );

          //then rotate around the camera's local X axis (vertical)
          cam.rotate_x( radians( ( 0.5f - mpos.y ) * 2 * 180 * vert_sensitivity ) );

          frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );

          warped = true;
        }
      }

      break;
    }
    case sf::Event::MouseButtonPressed:
    {
      if( ev.mouseButton.button == sf::Mouse::Right )
      {
        can_rotate = true;
      }

      break;
    }
    case sf::Event::MouseButtonReleased:
    {
      if( ev.mouseButton.button == sf::Mouse::Right )
      {
        can_rotate = false;
      }

      break;
    }
    case sf::Event::KeyPressed:
    {
                                if( ev.key.code == sf::Keyboard::Space )
                                {
                                  update_pm = !update_pm;
                                }

                                break;
    }
    default:
      break;
    }
  };

  /*
   * Render
   */

  sf::Clock timer;
  timer.restart();

  float elapsed_time = 0;

  frm.display( [&]
  {
    frm.handle_events( event_handler );
    float seconds = timer.getElapsedTime().asMilliseconds() * 0.001f;
    elapsed_time += seconds;
    
    if( seconds > 0.01667 )
    {
      timer.restart();
    }

    if( seconds > 0.01667 && has_focus )
    {
      move_camera( cam, move_amount, seconds );
    }

    glMatrixMode( GL_MODELVIEW );
    glLoadMatrixf( &cam.get_matrix()[0].x );

    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( &the_frame.projection_matrix[0].x );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //draw reference grid
    glUseProgram( 0 );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //WIREFRAME
    glDisable( GL_TEXTURE_2D );

    int size = 20;
    size /= 2;

    for( int x = -size; x < size + 1; x++ )
    {
      glBegin( GL_LINE_LOOP );
      glVertex3f( x, -2, -size );
      glVertex3f( x, -2, size );
      glEnd();
    };

    for( int y = -size; y < size + 1; y++ )
    {
      glBegin( GL_LINE_LOOP );
      glVertex3f( -size, -2, y );
      glVertex3f( size, -2, y );
      glEnd();
    };

    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////

    //render last, so dept sorting will be sorta-correct

    //update at 60hz
    if( seconds > 0.01667 )
    {
      if( update_pm )
      {
        pm.update( seconds );
      }

      //sort each particle system back-to-front
      auto ptr = pm.get( ps_id );
      auto sort_func = [&]( const particle& a, const particle& b ) -> bool
      {
        return dot( a.pos, cam.pos ) < dot( b.pos, cam.pos );
      };

      if( ptr )
      {
        std::sort( ptr->get_particle_iterator(), ptr->get_particle_iterator_end(), sort_func );
      }

      ptr = pm.get( ps_id2 );

      if( ptr )
      {
        std::sort( ptr->get_particle_iterator(), ptr->get_particle_iterator_end(), sort_func );
      }
    }

    auto render_func = []( particle_emitter* ptr, const camera<float>& cam, GLuint tex ) -> int
    {
      if( ptr )
      {
        auto ps_it = ptr->get_particle_iterator();
        auto ps_it_end = ptr->get_particle_iterator_end();

        //vec3 fwd = vec3( 0, 0, 1 );
        vec3 fwd = cam.view_dir;
        //vec3 up = vec3( 0, 1, 0 );
        vec3 up = cam.up_vector;
        vec3 right = cross( cam.view_dir, cam.up_vector );

        glUseProgram( 0 );
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        glEnable( GL_TEXTURE_2D );
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, tex );
        glEnable( GL_BLEND );

        if( ptr->is_additive )
        {
          //additive blending
          glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        }
        else
        {
          //normal blending
          glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }
        
        glDepthMask( false );

        int counter = 0;

        for( ; ps_it != ps_it_end; ++ps_it )
        {
          glColor4f( ps_it->color.x, ps_it->color.y, ps_it->color.z, ps_it->opacity );

          vec3 yaxis;
          if( ptr->is_stretched )
          {
            yaxis = normalize( ps_it->vel ) * 5;
          }
          else
          {
            yaxis = up;
          }

          vec3 zaxis = fwd;
          vec3 xaxis = normalize( cross( zaxis, yaxis ) );

          vec3 to_ur = normalize( xaxis + yaxis );
          //vec3 to_ur = ( right + up );
          vec3 to_ll = -to_ur;
          vec3 to_lr = normalize( xaxis - yaxis );
          //vec3 to_lr = ( right - up );
          vec3 to_ul = -to_lr;

          glBegin( GL_QUADS );
          vec3 ll = ps_it->pos + to_ll * ps_it->size;
          vec3 lr = ps_it->pos + to_lr * ps_it->size;
          vec3 ul = ps_it->pos + to_ul * ps_it->size;
          vec3 ur = ps_it->pos + to_ur * ps_it->size;

          //first, rotate the billboard to velocity direction, then stretch along the y axis

          glVertex3fv( &lr.x );
          glTexCoord2f( 1, 0 );
          glVertex3fv( &ur.x );
          glTexCoord2f( 1, 1 );
          glVertex3fv( &ul.x );
          glTexCoord2f( 0, 1 );
          glVertex3fv( &ll.x );
          glTexCoord2f( 0, 0 );

          glEnd();
          ++counter;
        }

        glDisable( GL_TEXTURE_2D );
        glDepthMask( true );
        glDisable( GL_BLEND );

        return counter;
      }
    };

    int particles_rendered = 0;

    //render particles
    auto ptr = pm.get( ps_id );
    particles_rendered += render_func( ptr, cam, tex );
    ptr = pm.get( ps_id2 );
    particles_rendered += render_func( ptr, cam, tex );

    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////

    frm.set_title( "Particles_rendered: " + std::to_string( particles_rendered ) );

    frm.get_opengl_error();
  }, silent );

  return 0;
}